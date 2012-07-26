/*
 * msvcrt.dll file functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2004 Eric Pouech
 * Copyright 2004 Juan Lang
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
 *
 * TODO
 * Use the file flag hints O_SEQUENTIAL, O_RANDOM, O_SHORT_LIVED
 */

#include "config.h"
#include "wine/port.h"

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "mtdll.h"

#include "wine/unicode.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* for stat mode, permissions apply to all,owner and group */
#define ALL_S_IREAD  (MSVCRT__S_IREAD  | (MSVCRT__S_IREAD  >> 3) | (MSVCRT__S_IREAD  >> 6))
#define ALL_S_IWRITE (MSVCRT__S_IWRITE | (MSVCRT__S_IWRITE >> 3) | (MSVCRT__S_IWRITE >> 6))
#define ALL_S_IEXEC  (MSVCRT__S_IEXEC  | (MSVCRT__S_IEXEC  >> 3) | (MSVCRT__S_IEXEC  >> 6))

/* _access() bit flags FIXME: incomplete */
#define MSVCRT_W_OK      0x02
#define MSVCRT_R_OK      0x04

/* values for wxflag in file descriptor */
#define WX_OPEN           0x01
#define WX_ATEOF          0x02
#define WX_READEOF        0x04  /* like ATEOF, but for underlying file rather than buffer */
#define WX_READCR         0x08  /* underlying file is at \r */
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TEXT           0x80

/* FIXME: this should be allocated dynamically */
#define MSVCRT_MAX_FILES 2048
#define MSVCRT_FD_BLOCK_SIZE 32

/* ioinfo structure size is different in msvcrXX.dll's */
typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    char                unk1;
    BOOL                crit_init;
    CRITICAL_SECTION    crit;
} ioinfo;

/*********************************************************************
 *		__pioinfo (MSVCRT.@)
 * array of pointers to ioinfo arrays [32]
 */
ioinfo * MSVCRT___pioinfo[MSVCRT_MAX_FILES/MSVCRT_FD_BLOCK_SIZE] = { 0 };

/*********************************************************************
 *		__badioinfo (MSVCRT.@)
 */
ioinfo MSVCRT___badioinfo = { INVALID_HANDLE_VALUE, WX_TEXT };

static int MSVCRT_fdstart = 3; /* first unallocated fd */
static int MSVCRT_fdend = 3; /* highest allocated fd */

typedef struct {
    MSVCRT_FILE file;
    CRITICAL_SECTION crit;
} file_crit;

MSVCRT_FILE MSVCRT__iob[_IOB_ENTRIES] = { { 0 } };
static file_crit* MSVCRT_fstream[MSVCRT_MAX_FILES/MSVCRT_FD_BLOCK_SIZE];
static int MSVCRT_max_streams = 512, MSVCRT_stream_idx;

/* INTERNAL: process umask */
static int MSVCRT_umask = 0;

/* INTERNAL: static data for tmpnam and _wtmpname functions */
static int tmpnam_unique;

static const unsigned int EXE = 'e' << 16 | 'x' << 8 | 'e';
static const unsigned int BAT = 'b' << 16 | 'a' << 8 | 't';
static const unsigned int CMD = 'c' << 16 | 'm' << 8 | 'd';
static const unsigned int COM = 'c' << 16 | 'o' << 8 | 'm';

#define TOUL(x) (ULONGLONG)(x)
static const ULONGLONG WCEXE = TOUL('e') << 32 | TOUL('x') << 16 | TOUL('e');
static const ULONGLONG WCBAT = TOUL('b') << 32 | TOUL('a') << 16 | TOUL('t');
static const ULONGLONG WCCMD = TOUL('c') << 32 | TOUL('m') << 16 | TOUL('d');
static const ULONGLONG WCCOM = TOUL('c') << 32 | TOUL('o') << 16 | TOUL('m');

/* This critical section protects the tables MSVCRT___pioinfo and MSVCRT_fstreams,
 * and their related indexes, MSVCRT_fdstart, MSVCRT_fdend,
 * and MSVCRT_stream_idx, from race conditions.
 * It doesn't protect against race conditions manipulating the underlying files
 * or flags; doing so would probably be better accomplished with per-file
 * protection, rather than locking the whole table for every change.
 */
static CRITICAL_SECTION MSVCRT_file_cs;
static CRITICAL_SECTION_DEBUG MSVCRT_file_cs_debug =
{
    0, 0, &MSVCRT_file_cs,
    { &MSVCRT_file_cs_debug.ProcessLocksList, &MSVCRT_file_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": MSVCRT_file_cs") }
};
static CRITICAL_SECTION MSVCRT_file_cs = { &MSVCRT_file_cs_debug, -1, 0, 0, 0, 0 };
#define LOCK_FILES()    do { EnterCriticalSection(&MSVCRT_file_cs); } while (0)
#define UNLOCK_FILES()  do { LeaveCriticalSection(&MSVCRT_file_cs); } while (0)

static void msvcrt_stat64_to_stat(const struct MSVCRT__stat64 *buf64, struct MSVCRT__stat *buf)
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

static void msvcrt_stat64_to_stati64(const struct MSVCRT__stat64 *buf64, struct MSVCRT__stati64 *buf)
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

static void time_to_filetime( MSVCRT___time64_t time, FILETIME *ft )
{
    /* 1601 to 1970 is 369 years plus 89 leap days */
    static const __int64 secs_1601_to_1970 = ((369 * 365 + 89) * (__int64)86400);

    __int64 ticks = (time + secs_1601_to_1970) * 10000000;
    ft->dwHighDateTime = ticks >> 32;
    ft->dwLowDateTime = ticks;
}

static inline ioinfo* msvcrt_get_ioinfo(int fd)
{
    ioinfo *ret = NULL;
    if(fd < MSVCRT_MAX_FILES)
        ret = MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE];
    if(!ret)
        return &MSVCRT___badioinfo;

    return ret + (fd%MSVCRT_FD_BLOCK_SIZE);
}

static inline MSVCRT_FILE* msvcrt_get_file(int i)
{
    file_crit *ret;

    if(i >= MSVCRT_max_streams)
        return NULL;

    if(i < _IOB_ENTRIES)
        return &MSVCRT__iob[i];

    ret = MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE];
    if(!ret) {
        MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE] = MSVCRT_calloc(MSVCRT_FD_BLOCK_SIZE, sizeof(file_crit));
        if(!MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE]) {
            ERR("out of memory\n");
            *MSVCRT__errno() = MSVCRT_ENOMEM;
            return NULL;
        }

        ret = MSVCRT_fstream[i/MSVCRT_FD_BLOCK_SIZE] + (i%MSVCRT_FD_BLOCK_SIZE);
    } else
        ret += i%MSVCRT_FD_BLOCK_SIZE;

    return &ret->file;
}

static inline BOOL msvcrt_is_valid_fd(int fd)
{
    return fd >= 0 && fd < MSVCRT_fdend && (msvcrt_get_ioinfo(fd)->wxflag & WX_OPEN);
}

/* INTERNAL: Get the HANDLE for a fd
 * This doesn't lock the table, because a failure will result in
 * INVALID_HANDLE_VALUE being returned, which should be handled correctly.  If
 * it returns a valid handle which is about to be closed, a subsequent call
 * will fail, most likely in a sane way.
 */
static HANDLE msvcrt_fdtoh(int fd)
{
  if (!msvcrt_is_valid_fd(fd))
  {
    WARN(":fd (%d) - no handle!\n",fd);
    *MSVCRT___doserrno() = 0;
    *MSVCRT__errno() = MSVCRT_EBADF;
    return INVALID_HANDLE_VALUE;
  }
  if (msvcrt_get_ioinfo(fd)->handle == INVALID_HANDLE_VALUE)
      WARN("returning INVALID_HANDLE_VALUE for %d\n", fd);
  return msvcrt_get_ioinfo(fd)->handle;
}

/* INTERNAL: free a file entry fd */
static void msvcrt_free_fd(int fd)
{
  HANDLE old_handle;
  ioinfo *fdinfo;

  LOCK_FILES();
  fdinfo = msvcrt_get_ioinfo(fd);
  old_handle = fdinfo->handle;
  if(fdinfo != &MSVCRT___badioinfo)
  {
    fdinfo->handle = INVALID_HANDLE_VALUE;
    fdinfo->wxflag = 0;
  }
  TRACE(":fd (%d) freed\n",fd);
  if (fd < 3) /* don't use 0,1,2 for user files */
  {
    switch (fd)
    {
    case 0:
        if (GetStdHandle(STD_INPUT_HANDLE) == old_handle) SetStdHandle(STD_INPUT_HANDLE, 0);
        break;
    case 1:
        if (GetStdHandle(STD_OUTPUT_HANDLE) == old_handle) SetStdHandle(STD_OUTPUT_HANDLE, 0);
        break;
    case 2:
        if (GetStdHandle(STD_ERROR_HANDLE) == old_handle) SetStdHandle(STD_ERROR_HANDLE, 0);
        break;
    }
  }
  else
  {
    if (fd == MSVCRT_fdend - 1)
      MSVCRT_fdend--;
    if (fd < MSVCRT_fdstart)
      MSVCRT_fdstart = fd;
  }
  UNLOCK_FILES();
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE, starting from fd */
/* caller must hold the files lock */
static int msvcrt_alloc_fd_from(HANDLE hand, int flag, int fd)
{
  ioinfo *fdinfo;

  if (fd >= MSVCRT_MAX_FILES)
  {
    WARN(":files exhausted!\n");
    *MSVCRT__errno() = MSVCRT_ENFILE;
    return -1;
  }

  fdinfo = msvcrt_get_ioinfo(fd);
  if(fdinfo == &MSVCRT___badioinfo) {
    int i;

    MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE] = MSVCRT_calloc(MSVCRT_FD_BLOCK_SIZE, sizeof(ioinfo));
    if(!MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE]) {
      WARN(":out of memory!\n");
      *MSVCRT__errno() = MSVCRT_ENOMEM;
      return -1;
    }

    for(i=0; i<MSVCRT_FD_BLOCK_SIZE; i++)
      MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE][i].handle = INVALID_HANDLE_VALUE;

    fdinfo = msvcrt_get_ioinfo(fd);
  }

  fdinfo->handle = hand;
  fdinfo->wxflag = WX_OPEN | (flag & (WX_DONTINHERIT | WX_APPEND | WX_TEXT));

  /* locate next free slot */
  if (fd == MSVCRT_fdstart && fd == MSVCRT_fdend)
    MSVCRT_fdstart = MSVCRT_fdend + 1;
  else
    while (MSVCRT_fdstart < MSVCRT_fdend &&
      msvcrt_get_ioinfo(MSVCRT_fdstart)->handle != INVALID_HANDLE_VALUE)
      MSVCRT_fdstart++;
  /* update last fd in use */
  if (fd >= MSVCRT_fdend)
    MSVCRT_fdend = fd + 1;
  TRACE("fdstart is %d, fdend is %d\n", MSVCRT_fdstart, MSVCRT_fdend);

  switch (fd)
  {
  case 0: SetStdHandle(STD_INPUT_HANDLE,  hand); break;
  case 1: SetStdHandle(STD_OUTPUT_HANDLE, hand); break;
  case 2: SetStdHandle(STD_ERROR_HANDLE,  hand); break;
  }

  return fd;
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE */
static int msvcrt_alloc_fd(HANDLE hand, int flag)
{
  int ret;

  LOCK_FILES();
  TRACE(":handle (%p) allocating fd (%d)\n",hand,MSVCRT_fdstart);
  ret = msvcrt_alloc_fd_from(hand, flag, MSVCRT_fdstart);
  UNLOCK_FILES();
  return ret;
}

/* INTERNAL: Allocate a FILE* for an fd slot */
/* caller must hold the files lock */
static MSVCRT_FILE* msvcrt_alloc_fp(void)
{
  unsigned int i;
  MSVCRT_FILE *file;

  for (i = 3; i < MSVCRT_max_streams; i++)
  {
    file = msvcrt_get_file(i);
    if (!file)
      return NULL;

    if (file->_flag == 0)
    {
      if (i == MSVCRT_stream_idx)
      {
          if (file<MSVCRT__iob || file>=MSVCRT__iob+_IOB_ENTRIES)
          {
              InitializeCriticalSection(&((file_crit*)file)->crit);
              ((file_crit*)file)->crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": file_crit.crit");
          }
          MSVCRT_stream_idx++;
      }
      return file;
    }
  }

  return NULL;
}

/* INTERNAL: initialize a FILE* from an open fd */
static int msvcrt_init_fp(MSVCRT_FILE* file, int fd, unsigned stream_flags)
{
  TRACE(":fd (%d) allocating FILE*\n",fd);
  if (!msvcrt_is_valid_fd(fd))
  {
    WARN(":invalid fd %d\n",fd);
    *MSVCRT___doserrno() = 0;
    *MSVCRT__errno() = MSVCRT_EBADF;
    return -1;
  }
  memset(file, 0, sizeof(*file));
  file->_file = fd;
  file->_flag = stream_flags;

  TRACE(":got FILE* (%p)\n",file);
  return 0;
}

/* INTERNAL: Create an inheritance data block (for spawned process)
 * The inheritance block is made of:
 *      00      int     nb of file descriptor (NBFD)
 *      04      char    file flags (wxflag): repeated for each fd
 *      4+NBFD  HANDLE  file handle: repeated for each fd
 */
unsigned msvcrt_create_io_inherit_block(WORD *size, BYTE **block)
{
  int         fd;
  char*       wxflag_ptr;
  HANDLE*     handle_ptr;
  ioinfo*     fdinfo;

  *size = sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * MSVCRT_fdend;
  *block = MSVCRT_calloc(*size, 1);
  if (!*block)
  {
    *size = 0;
    return FALSE;
  }
  wxflag_ptr = (char*)*block + sizeof(unsigned);
  handle_ptr = (HANDLE*)(wxflag_ptr + MSVCRT_fdend * sizeof(char));

  *(unsigned*)*block = MSVCRT_fdend;
  for (fd = 0; fd < MSVCRT_fdend; fd++)
  {
    /* to be inherited, we need it to be open, and that DONTINHERIT isn't set */
    fdinfo = msvcrt_get_ioinfo(fd);
    if ((fdinfo->wxflag & (WX_OPEN | WX_DONTINHERIT)) == WX_OPEN)
    {
      *wxflag_ptr = fdinfo->wxflag;
      *handle_ptr = fdinfo->handle;
    }
    else
    {
      *wxflag_ptr = 0;
      *handle_ptr = INVALID_HANDLE_VALUE;
    }
    wxflag_ptr++; handle_ptr++;
  } 
  return TRUE;
}

/* INTERNAL: Set up all file descriptors, 
 * as well as default streams (stdin, stderr and stdout) 
 */
void msvcrt_init_io(void)
{
  STARTUPINFOA  si;
  int           i;
  ioinfo        *fdinfo;

  GetStartupInfoA(&si);
  if (si.cbReserved2 >= sizeof(unsigned int) && si.lpReserved2 != NULL)
  {
    BYTE*       wxflag_ptr;
    HANDLE*     handle_ptr;
    unsigned int count;

    count = *(unsigned*)si.lpReserved2;
    wxflag_ptr = si.lpReserved2 + sizeof(unsigned);
    handle_ptr = (HANDLE*)(wxflag_ptr + count);

    count = min(count, (si.cbReserved2 - sizeof(unsigned)) / (sizeof(HANDLE) + 1));
    count = min(count, MSVCRT_MAX_FILES);
    for (i = 0; i < count; i++)
    {
      if ((*wxflag_ptr & WX_OPEN) && *handle_ptr != INVALID_HANDLE_VALUE)
        msvcrt_alloc_fd_from(*handle_ptr, *wxflag_ptr, i);

      wxflag_ptr++; handle_ptr++;
    }
    MSVCRT_fdend = max( 3, count );
    for (MSVCRT_fdstart = 3; MSVCRT_fdstart < MSVCRT_fdend; MSVCRT_fdstart++)
        if (msvcrt_get_ioinfo(MSVCRT_fdstart)->handle == INVALID_HANDLE_VALUE) break;
  }

  if(!MSVCRT___pioinfo[0])
      msvcrt_alloc_fd_from(INVALID_HANDLE_VALUE, 0, 3);

  fdinfo = msvcrt_get_ioinfo(0);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE)
  {
      HANDLE std = GetStdHandle(STD_INPUT_HANDLE);
      if (std != INVALID_HANDLE_VALUE && DuplicateHandle(GetCurrentProcess(), std,
                                                         GetCurrentProcess(), &fdinfo->handle,
                                                         0, TRUE, DUPLICATE_SAME_ACCESS))
          fdinfo->wxflag = WX_OPEN | WX_TEXT;
  }

  fdinfo = msvcrt_get_ioinfo(1);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE)
  {
      HANDLE std = GetStdHandle(STD_OUTPUT_HANDLE);
      if (std != INVALID_HANDLE_VALUE && DuplicateHandle(GetCurrentProcess(), std,
                                                         GetCurrentProcess(), &fdinfo->handle,
                                                         0, TRUE, DUPLICATE_SAME_ACCESS))
          fdinfo->wxflag = WX_OPEN | WX_TEXT;
  }

  fdinfo = msvcrt_get_ioinfo(2);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE)
  {
      HANDLE std = GetStdHandle(STD_ERROR_HANDLE);
      if (std != INVALID_HANDLE_VALUE && DuplicateHandle(GetCurrentProcess(), std,
                                                         GetCurrentProcess(), &fdinfo->handle,
                                                         0, TRUE, DUPLICATE_SAME_ACCESS))
          fdinfo->wxflag = WX_OPEN | WX_TEXT;
  }

  TRACE(":handles (%p)(%p)(%p)\n", msvcrt_get_ioinfo(0)->handle,
	msvcrt_get_ioinfo(1)->handle, msvcrt_get_ioinfo(2)->handle);

  memset(MSVCRT__iob,0,3*sizeof(MSVCRT_FILE));
  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    MSVCRT__iob[i]._file = i;
    MSVCRT__iob[i]._tmpfname = NULL;
    MSVCRT__iob[i]._flag = (i == 0) ? MSVCRT__IOREAD : MSVCRT__IOWRT;
  }
  MSVCRT_stream_idx = 3;
}

/* INTERNAL: Flush stdio file buffer */
static int msvcrt_flush_buffer(MSVCRT_FILE* file)
{
  if(file->_bufsiz) {
        int cnt=file->_ptr-file->_base;
        if(cnt>0 && MSVCRT__write(file->_file, file->_base, cnt) != cnt) {
            file->_flag |= MSVCRT__IOERR;
            return MSVCRT_EOF;
        }
        file->_ptr=file->_base;
        file->_cnt=file->_bufsiz;
  }
  return 0;
}

/* INTERNAL: Allocate stdio file buffer */
static void msvcrt_alloc_buffer(MSVCRT_FILE* file)
{
	file->_base = MSVCRT_calloc(MSVCRT_BUFSIZ,1);
	if(file->_base) {
		file->_bufsiz = MSVCRT_BUFSIZ;
		file->_flag |= MSVCRT__IOMYBUF;
	} else {
		file->_base = (char*)(&file->_charbuf);
		/* put here 2 ??? */
		file->_bufsiz = sizeof(file->_charbuf);
	}
	file->_ptr = file->_base;
	file->_cnt = 0;
}

/* INTERNAL: Convert integer to base32 string (0-9a-v), 0 becomes "" */
static int msvcrt_int_to_base32(int num, char *str)
{
  char *p;
  int n = num;
  int digits = 0;

  while (n != 0)
  {
    n >>= 5;
    digits++;
  }
  p = str + digits;
  *p = 0;
  while (--p >= str)
  {
    *p = (num & 31) + '0';
    if (*p > '9')
      *p += ('a' - '0' - 10);
    num >>= 5;
  }

  return digits;
}

/* INTERNAL: wide character version of msvcrt_int_to_base32 */
static int msvcrt_int_to_base32_w(int num, MSVCRT_wchar_t *str)
{
    MSVCRT_wchar_t *p;
    int n = num;
    int digits = 0;

    while (n != 0)
    {
        n >>= 5;
        digits++;
    }
    p = str + digits;
    *p = 0;
    while (--p >= str)
    {
        *p = (num & 31) + '0';
        if (*p > '9')
            *p += ('a' - '0' - 10);
        num >>= 5;
    }

    return digits;
}

/*********************************************************************
 *		__iob_func(MSVCRT.@)
 */
MSVCRT_FILE * CDECL MSVCRT___iob_func(void)
{
 return &MSVCRT__iob[0];
}

/*********************************************************************
 *		_access (MSVCRT.@)
 */
int CDECL MSVCRT__access(const char *filename, int mode)
{
  DWORD attr = GetFileAttributesA(filename);

  TRACE("(%s,%d) %d\n",filename,mode,attr);

  if (!filename || attr == INVALID_FILE_ATTRIBUTES)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & MSVCRT_W_OK))
  {
    msvcrt_set_errno(ERROR_ACCESS_DENIED);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_access_s (MSVCRT.@)
 */
int CDECL _access_s(const char *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL)) return -1;
  if (!MSVCRT_CHECK_PMT((mode & ~(MSVCRT_R_OK | MSVCRT_W_OK)) == 0)) return -1;

  return MSVCRT__access(filename, mode);
}

/*********************************************************************
 *		_waccess (MSVCRT.@)
 */
int CDECL MSVCRT__waccess(const MSVCRT_wchar_t *filename, int mode)
{
  DWORD attr = GetFileAttributesW(filename);

  TRACE("(%s,%d) %d\n",debugstr_w(filename),mode,attr);

  if (!filename || attr == INVALID_FILE_ATTRIBUTES)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & MSVCRT_W_OK))
  {
    msvcrt_set_errno(ERROR_ACCESS_DENIED);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_waccess_s (MSVCRT.@)
 */
int CDECL _waccess_s(const MSVCRT_wchar_t *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL)) return -1;
  if (!MSVCRT_CHECK_PMT((mode & ~(MSVCRT_R_OK | MSVCRT_W_OK)) == 0)) return -1;

  return MSVCRT__waccess(filename, mode);
}

/*********************************************************************
 *		_chmod (MSVCRT.@)
 */
int CDECL MSVCRT__chmod(const char *path, int flags)
{
  DWORD oldFlags = GetFileAttributesA(path);

  if (oldFlags != INVALID_FILE_ATTRIBUTES)
  {
    DWORD newFlags = (flags & MSVCRT__S_IWRITE)? oldFlags & ~FILE_ATTRIBUTE_READONLY:
      oldFlags | FILE_ATTRIBUTE_READONLY;

    if (newFlags == oldFlags || SetFileAttributesA(path, newFlags))
      return 0;
  }
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wchmod (MSVCRT.@)
 */
int CDECL MSVCRT__wchmod(const MSVCRT_wchar_t *path, int flags)
{
  DWORD oldFlags = GetFileAttributesW(path);

  if (oldFlags != INVALID_FILE_ATTRIBUTES)
  {
    DWORD newFlags = (flags & MSVCRT__S_IWRITE)? oldFlags & ~FILE_ATTRIBUTE_READONLY:
      oldFlags | FILE_ATTRIBUTE_READONLY;

    if (newFlags == oldFlags || SetFileAttributesW(path, newFlags))
      return 0;
  }
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_unlink (MSVCRT.@)
 */
int CDECL MSVCRT__unlink(const char *path)
{
  TRACE("%s\n",debugstr_a(path));
  if(DeleteFileA(path))
    return 0;
  TRACE("failed (%d)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wunlink (MSVCRT.@)
 */
int CDECL MSVCRT__wunlink(const MSVCRT_wchar_t *path)
{
  TRACE("(%s)\n",debugstr_w(path));
  if(DeleteFileW(path))
    return 0;
  TRACE("failed (%d)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/* flush_all_buffers calls MSVCRT_fflush which calls flush_all_buffers */
int CDECL MSVCRT_fflush(MSVCRT_FILE* file);

/* INTERNAL: Flush all stream buffer */
static int msvcrt_flush_all_buffers(int mask)
{
  int i, num_flushed = 0;
  MSVCRT_FILE *file;

  LOCK_FILES();
  for (i = 3; i < MSVCRT_stream_idx; i++) {
    file = msvcrt_get_file(i);

    if (file->_flag)
    {
      if(file->_flag & mask) {
	MSVCRT_fflush(file);
        num_flushed++;
      }
    }
  }
  UNLOCK_FILES();

  TRACE(":flushed (%d) handles\n",num_flushed);
  return num_flushed;
}

/*********************************************************************
 *		_flushall (MSVCRT.@)
 */
int CDECL MSVCRT__flushall(void)
{
    return msvcrt_flush_all_buffers(MSVCRT__IOWRT | MSVCRT__IOREAD);
}

/*********************************************************************
 *		fflush (MSVCRT.@)
 */
int CDECL MSVCRT_fflush(MSVCRT_FILE* file)
{
    if(!file) {
        msvcrt_flush_all_buffers(MSVCRT__IOWRT);
    } else if(file->_flag & MSVCRT__IOWRT) {
        int res;

        MSVCRT__lock_file(file);
        res = msvcrt_flush_buffer(file);
        MSVCRT__unlock_file(file);

        return res;
    } else if(file->_flag & MSVCRT__IOREAD) {
        MSVCRT__lock_file(file);
        file->_cnt = 0;
        file->_ptr = file->_base;
        MSVCRT__unlock_file(file);

        return 0;
    }
    return 0;
}

/*********************************************************************
 *		_close (MSVCRT.@)
 */
int CDECL MSVCRT__close(int fd)
{
  HANDLE hand;
  int ret;

  LOCK_FILES();
  hand = msvcrt_fdtoh(fd);
  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (!msvcrt_is_valid_fd(fd)) {
    ret = -1;
  } else {
    msvcrt_free_fd(fd);
    ret = CloseHandle(hand) ? 0 : -1;
    if (ret) {
      WARN(":failed-last error (%d)\n",GetLastError());
      msvcrt_set_errno(GetLastError());
    }
  }
  UNLOCK_FILES();
  TRACE(":ok\n");
  return ret;
}

/*********************************************************************
 *		_commit (MSVCRT.@)
 */
int CDECL MSVCRT__commit(int fd)
{
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (!FlushFileBuffers(hand))
  {
    if (GetLastError() == ERROR_INVALID_HANDLE)
    {
      /* FlushFileBuffers fails for console handles
       * so we ignore this error.
       */
      return 0;
    }
    TRACE(":failed-last error (%d)\n",GetLastError());
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  TRACE(":ok\n");
  return 0;
}

/*********************************************************************
 *		_dup2 (MSVCRT.@)
 * NOTES
 * MSDN isn't clear on this point, but the remarks for _pipe
 * indicate file descriptors duplicated with _dup and _dup2 are always
 * inheritable.
 */
int CDECL MSVCRT__dup2(int od, int nd)
{
  int ret;

  TRACE("(od=%d, nd=%d)\n", od, nd);
  LOCK_FILES();
  if (nd < MSVCRT_MAX_FILES && nd >= 0 && msvcrt_is_valid_fd(od))
  {
    HANDLE handle;

    if (DuplicateHandle(GetCurrentProcess(), msvcrt_get_ioinfo(od)->handle,
     GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
      int wxflag = msvcrt_get_ioinfo(od)->wxflag & ~MSVCRT__O_NOINHERIT;

      if (msvcrt_is_valid_fd(nd))
        MSVCRT__close(nd);
      ret = msvcrt_alloc_fd_from(handle, wxflag, nd);
      if (ret == -1)
      {
        CloseHandle(handle);
        *MSVCRT__errno() = MSVCRT_EMFILE;
      }
      else
      {
        /* _dup2 returns 0, not nd, on success */
        ret = 0;
      }
    }
    else
    {
      ret = -1;
      msvcrt_set_errno(GetLastError());
    }
  }
  else
  {
    *MSVCRT__errno() = MSVCRT_EBADF;
    ret = -1;
  }
  UNLOCK_FILES();
  return ret;
}

/*********************************************************************
 *		_dup (MSVCRT.@)
 */
int CDECL MSVCRT__dup(int od)
{
  int fd, ret;
 
  LOCK_FILES();
  fd = MSVCRT_fdstart;
  if (MSVCRT__dup2(od, fd) == 0)
    ret = fd;
  else
    ret = -1;
  UNLOCK_FILES();
  return ret;
}

/*********************************************************************
 *		_eof (MSVCRT.@)
 */
int CDECL MSVCRT__eof(int fd)
{
  DWORD curpos,endpos;
  LONG hcurpos,hendpos;
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (msvcrt_get_ioinfo(fd)->wxflag & WX_ATEOF) return TRUE;

  /* Otherwise we do it the hard way */
  hcurpos = hendpos = 0;
  curpos = SetFilePointer(hand, 0, &hcurpos, FILE_CURRENT);
  endpos = SetFilePointer(hand, 0, &hendpos, FILE_END);

  if (curpos == endpos && hcurpos == hendpos)
  {
    /* FIXME: shouldn't WX_ATEOF be set here? */
    return TRUE;
  }

  SetFilePointer(hand, curpos, &hcurpos, FILE_BEGIN);
  return FALSE;
}

/*********************************************************************
 *		_fcloseall (MSVCRT.@)
 */
int CDECL MSVCRT__fcloseall(void)
{
  int num_closed = 0, i;
  MSVCRT_FILE *file;

  LOCK_FILES();
  for (i = 3; i < MSVCRT_stream_idx; i++) {
    file = msvcrt_get_file(i);

    if (file->_flag && !MSVCRT_fclose(file))
      num_closed++;
  }
  UNLOCK_FILES();

  TRACE(":closed (%d) handles\n",num_closed);
  return num_closed;
}

/* free everything on process exit */
void msvcrt_free_io(void)
{
    int i;

    MSVCRT__fcloseall();
    /* The Win32 _fcloseall() function explicitly doesn't close stdin,
     * stdout, and stderr (unlike GNU), so we need to fclose() them here
     * or they won't get flushed.
     */
    MSVCRT_fclose(&MSVCRT__iob[0]);
    MSVCRT_fclose(&MSVCRT__iob[1]);
    MSVCRT_fclose(&MSVCRT__iob[2]);

    for(i=0; i<sizeof(MSVCRT___pioinfo)/sizeof(MSVCRT___pioinfo[0]); i++)
        MSVCRT_free(MSVCRT___pioinfo[i]);

    for(i=0; i<MSVCRT_stream_idx; i++)
    {
        MSVCRT_FILE *file = msvcrt_get_file(i);
        if(file<MSVCRT__iob || file>=MSVCRT__iob+_IOB_ENTRIES)
        {
            ((file_crit*)file)->crit.DebugInfo->Spare[0] = 0;
            DeleteCriticalSection(&((file_crit*)file)->crit);
        }
    }

    for(i=0; i<sizeof(MSVCRT_fstream)/sizeof(MSVCRT_fstream[0]); i++)
        MSVCRT_free(MSVCRT_fstream[i]);

    DeleteCriticalSection(&MSVCRT_file_cs);
}

/*********************************************************************
 *		_lseeki64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__lseeki64(int fd, __int64 offset, int whence)
{
  HANDLE hand = msvcrt_fdtoh(fd);
  LARGE_INTEGER ofs;

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (whence < 0 || whence > 2)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  TRACE(":fd (%d) to %s pos %s\n",
        fd,wine_dbgstr_longlong(offset),
        (whence==SEEK_SET)?"SEEK_SET":
        (whence==SEEK_CUR)?"SEEK_CUR":
        (whence==SEEK_END)?"SEEK_END":"UNKNOWN");

  /* The MoleBox protection scheme expects msvcrt to use SetFilePointer only,
   * so a LARGE_INTEGER offset cannot be passed directly via SetFilePointerEx. */
  ofs.QuadPart = offset;
  if ((ofs.u.LowPart = SetFilePointer(hand, ofs.u.LowPart, &ofs.u.HighPart, whence)) != INVALID_SET_FILE_POINTER ||
      GetLastError() == ERROR_SUCCESS)
  {
    msvcrt_get_ioinfo(fd)->wxflag &= ~(WX_ATEOF|WX_READEOF);
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    return ofs.QuadPart;
  }
  TRACE(":error-last error (%d)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_lseek (MSVCRT.@)
 */
LONG CDECL MSVCRT__lseek(int fd, LONG offset, int whence)
{
    return MSVCRT__lseeki64(fd, offset, whence);
}

/*********************************************************************
 *              _lock_file (MSVCRT.@)
 */
void CDECL MSVCRT__lock_file(MSVCRT_FILE *file)
{
    if(file>=MSVCRT__iob && file<MSVCRT__iob+_IOB_ENTRIES)
        _lock(_STREAM_LOCKS+(file-MSVCRT__iob));
    else
        EnterCriticalSection(&((file_crit*)file)->crit);
}

/*********************************************************************
 *              _unlock_file (MSVCRT.@)
 */
void CDECL MSVCRT__unlock_file(MSVCRT_FILE *file)
{
    if(file>=MSVCRT__iob && file<MSVCRT__iob+_IOB_ENTRIES)
        _unlock(_STREAM_LOCKS+(file-MSVCRT__iob));
    else
        LeaveCriticalSection(&((file_crit*)file)->crit);
}

/*********************************************************************
 *		_locking (MSVCRT.@)
 *
 * This is untested; the underlying LockFile doesn't work yet.
 */
int CDECL MSVCRT__locking(int fd, int mode, LONG nbytes)
{
  BOOL ret;
  DWORD cur_locn;
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (mode < 0 || mode > 4)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  TRACE(":fd (%d) by 0x%08x mode %s\n",
        fd,nbytes,(mode==MSVCRT__LK_UNLCK)?"_LK_UNLCK":
        (mode==MSVCRT__LK_LOCK)?"_LK_LOCK":
        (mode==MSVCRT__LK_NBLCK)?"_LK_NBLCK":
        (mode==MSVCRT__LK_RLCK)?"_LK_RLCK":
        (mode==MSVCRT__LK_NBRLCK)?"_LK_NBRLCK":
                          "UNKNOWN");

  if ((cur_locn = SetFilePointer(hand, 0L, NULL, SEEK_CUR)) == INVALID_SET_FILE_POINTER)
  {
    FIXME ("Seek failed\n");
    *MSVCRT__errno() = MSVCRT_EINVAL; /* FIXME */
    return -1;
  }
  if (mode == MSVCRT__LK_LOCK || mode == MSVCRT__LK_RLCK)
  {
    int nretry = 10;
    ret = 1; /* just to satisfy gcc */
    while (nretry--)
    {
      ret = LockFile(hand, cur_locn, 0L, nbytes, 0L);
      if (ret) break;
      Sleep(1);
    }
  }
  else if (mode == MSVCRT__LK_UNLCK)
    ret = UnlockFile(hand, cur_locn, 0L, nbytes, 0L);
  else
    ret = LockFile(hand, cur_locn, 0L, nbytes, 0L);
  /* FIXME - what about error settings? */
  return ret ? 0 : -1;
}

/*********************************************************************
 *		_fseeki64 (MSVCRT.@)
 */
int CDECL MSVCRT__fseeki64(MSVCRT_FILE* file, __int64 offset, int whence)
{
  int ret;

  MSVCRT__lock_file(file);
  /* Flush output if needed */
  if(file->_flag & MSVCRT__IOWRT)
	msvcrt_flush_buffer(file);

  if(whence == SEEK_CUR && file->_flag & MSVCRT__IOREAD ) {
	offset -= file->_cnt;
	if (msvcrt_get_ioinfo(file->_file)->wxflag & WX_TEXT) {
		/* Black magic correction for CR removal */
		int i;
		for (i=0; i<file->_cnt; i++) {
			if (file->_ptr[i] == '\n')
				offset--;
		}
		/* Black magic when reading CR at buffer boundary*/
		if(msvcrt_get_ioinfo(file->_file)->wxflag & WX_READCR)
		    offset--;
	}
  }
  /* Discard buffered input */
  file->_cnt = 0;
  file->_ptr = file->_base;
  /* Reset direction of i/o */
  if(file->_flag & MSVCRT__IORW) {
        file->_flag &= ~(MSVCRT__IOREAD|MSVCRT__IOWRT);
  }
  /* Clear end of file flag */
  file->_flag &= ~MSVCRT__IOEOF;
  ret = (MSVCRT__lseeki64(file->_file,offset,whence) == -1)?-1:0;

  MSVCRT__unlock_file(file);
  return ret;
}

/*********************************************************************
 *		fseek (MSVCRT.@)
 */
int CDECL MSVCRT_fseek(MSVCRT_FILE* file, MSVCRT_long offset, int whence)
{
    return MSVCRT__fseeki64( file, offset, whence );
}

/*********************************************************************
 *		_chsize (MSVCRT.@)
 */
int CDECL MSVCRT__chsize(int fd, MSVCRT_long size)
{
    LONG cur, pos;
    HANDLE handle;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%d)\n", fd, size);

    LOCK_FILES();

    handle = msvcrt_fdtoh(fd);
    if (handle != INVALID_HANDLE_VALUE)
    {
        /* save the current file pointer */
        cur = MSVCRT__lseek(fd, 0, SEEK_CUR);
        if (cur >= 0)
        {
            pos = MSVCRT__lseek(fd, size, SEEK_SET);
            if (pos >= 0)
            {
                ret = SetEndOfFile(handle);
                if (!ret) msvcrt_set_errno(GetLastError());
            }

            /* restore the file pointer */
            MSVCRT__lseek(fd, cur, SEEK_SET);
        }
    }

    UNLOCK_FILES();
    return ret ? 0 : -1;
}

/*********************************************************************
 *		clearerr (MSVCRT.@)
 */
void CDECL MSVCRT_clearerr(MSVCRT_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);

  MSVCRT__lock_file(file);
  file->_flag &= ~(MSVCRT__IOERR | MSVCRT__IOEOF);
  MSVCRT__unlock_file(file);
}

/*********************************************************************
 *		rewind (MSVCRT.@)
 */
void CDECL MSVCRT_rewind(MSVCRT_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);

  MSVCRT__lock_file(file);
  MSVCRT_fseek(file, 0L, SEEK_SET);
  MSVCRT_clearerr(file);
  MSVCRT__unlock_file(file);
}

static int msvcrt_get_flags(const MSVCRT_wchar_t* mode, int *open_flags, int* stream_flags)
{
  int plus = strchrW(mode, '+') != NULL;

  switch(*mode++)
  {
  case 'R': case 'r':
    *open_flags = plus ? MSVCRT__O_RDWR : MSVCRT__O_RDONLY;
    *stream_flags = plus ? MSVCRT__IORW : MSVCRT__IOREAD;
    break;
  case 'W': case 'w':
    *open_flags = MSVCRT__O_CREAT | MSVCRT__O_TRUNC | (plus  ? MSVCRT__O_RDWR : MSVCRT__O_WRONLY);
    *stream_flags = plus ? MSVCRT__IORW : MSVCRT__IOWRT;
    break;
  case 'A': case 'a':
    *open_flags = MSVCRT__O_CREAT | MSVCRT__O_APPEND | (plus  ? MSVCRT__O_RDWR : MSVCRT__O_WRONLY);
    *stream_flags = plus ? MSVCRT__IORW : MSVCRT__IOWRT;
    break;
  default:
    MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  while (*mode)
    switch (*mode++)
    {
    case 'B': case 'b':
      *open_flags |=  MSVCRT__O_BINARY;
      *open_flags &= ~MSVCRT__O_TEXT;
      break;
    case 't':
      *open_flags |=  MSVCRT__O_TEXT;
      *open_flags &= ~MSVCRT__O_BINARY;
      break;
    case 'D':
      *open_flags |= MSVCRT__O_TEMPORARY;
      break;
    case 'T':
      *open_flags |= MSVCRT__O_SHORT_LIVED;
      break;
    case '+':
    case ' ':
      break;
    default:
      FIXME(":unknown flag %c not supported\n",mode[-1]);
    }
  return 0;
}

/*********************************************************************
 *		_fdopen (MSVCRT.@)
 */
MSVCRT_FILE* CDECL MSVCRT__fdopen(int fd, const char *mode)
{
    MSVCRT_FILE *ret;
    MSVCRT_wchar_t *modeW = NULL;

    if (mode && !(modeW = msvcrt_wstrdupa(mode))) return NULL;

    ret = MSVCRT__wfdopen(fd, modeW);

    MSVCRT_free(modeW);
    return ret;
}

/*********************************************************************
 *		_wfdopen (MSVCRT.@)
 */
MSVCRT_FILE* CDECL MSVCRT__wfdopen(int fd, const MSVCRT_wchar_t *mode)
{
  int open_flags, stream_flags;
  MSVCRT_FILE* file;

  if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1) return NULL;

  LOCK_FILES();
  if (!(file = msvcrt_alloc_fp()))
    file = NULL;
  else if (msvcrt_init_fp(file, fd, stream_flags) == -1)
  {
    file->_flag = 0;
    file = NULL;
  }
  else TRACE(":fd (%d) mode (%s) FILE* (%p)\n", fd, debugstr_w(mode), file);
  UNLOCK_FILES();

  return file;
}

/*********************************************************************
 *		_filelength (MSVCRT.@)
 */
LONG CDECL MSVCRT__filelength(int fd)
{
  LONG curPos = MSVCRT__lseek(fd, 0, SEEK_CUR);
  if (curPos != -1)
  {
    LONG endPos = MSVCRT__lseek(fd, 0, SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        MSVCRT__lseek(fd, curPos, SEEK_SET);
      return endPos;
    }
  }
  return -1;
}

/*********************************************************************
 *		_filelengthi64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__filelengthi64(int fd)
{
  __int64 curPos = MSVCRT__lseeki64(fd, 0, SEEK_CUR);
  if (curPos != -1)
  {
    __int64 endPos = MSVCRT__lseeki64(fd, 0, SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        MSVCRT__lseeki64(fd, curPos, SEEK_SET);
      return endPos;
    }
  }
  return -1;
}

/*********************************************************************
 *		_fileno (MSVCRT.@)
 */
int CDECL MSVCRT__fileno(MSVCRT_FILE* file)
{
  TRACE(":FILE* (%p) fd (%d)\n",file,file->_file);
  return file->_file;
}

/*********************************************************************
 *		_fstat64 (MSVCRT.@)
 */
int CDECL MSVCRT__fstat64(int fd, struct MSVCRT__stat64* buf)
{
  DWORD dw;
  DWORD type;
  BY_HANDLE_FILE_INFORMATION hfi;
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) stat (%p)\n",fd,buf);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (!buf)
  {
    WARN(":failed-NULL buf\n");
    msvcrt_set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct MSVCRT__stat64));
  type = GetFileType(hand);
  if (type == FILE_TYPE_PIPE)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = S_IFIFO;
    buf->st_nlink = 1;
  }
  else if (type == FILE_TYPE_CHAR)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = S_IFCHR;
    buf->st_nlink = 1;
  }
  else /* FILE_TYPE_DISK etc. */
  {
    if (!GetFileInformationByHandle(hand, &hfi))
    {
      WARN(":failed-last error (%d)\n",GetLastError());
      msvcrt_set_errno(ERROR_INVALID_PARAMETER);
      return -1;
    }
    buf->st_mode = S_IFREG | 0444;
    if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      buf->st_mode |= 0222;
    buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
    buf->st_atime = dw;
    RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
    buf->st_mtime = buf->st_ctime = dw;
    buf->st_nlink = hfi.nNumberOfLinks;
  }
  TRACE(":dwFileAttributes = 0x%x, mode set to 0x%x\n",hfi.dwFileAttributes,
   buf->st_mode);
  return 0;
}

/*********************************************************************
 *		_fstati64 (MSVCRT.@)
 */
int CDECL MSVCRT__fstati64(int fd, struct MSVCRT__stati64* buf)
{
  int ret;
  struct MSVCRT__stat64 buf64;

  ret = MSVCRT__fstat64(fd, &buf64);
  if (!ret)
    msvcrt_stat64_to_stati64(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_fstat (MSVCRT.@)
 */
int CDECL MSVCRT__fstat(int fd, struct MSVCRT__stat* buf)
{ int ret;
  struct MSVCRT__stat64 buf64;

  ret = MSVCRT__fstat64(fd, &buf64);
  if (!ret)
      msvcrt_stat64_to_stat(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_futime64 (MSVCRT.@)
 */
int CDECL _futime64(int fd, struct MSVCRT___utimbuf64 *t)
{
  HANDLE hand = msvcrt_fdtoh(fd);
  FILETIME at, wt;

  if (!t)
  {
      time_to_filetime( MSVCRT__time64(NULL), &at );
      wt = at;
  }
  else
  {
      time_to_filetime( t->actime, &at );
      time_to_filetime( t->modtime, &wt );
  }

  if (!SetFileTime(hand, NULL, &at, &wt))
  {
    msvcrt_set_errno(GetLastError());
    return -1 ;
  }
  return 0;
}

/*********************************************************************
 *		_futime32 (MSVCRT.@)
 */
int CDECL _futime32(int fd, struct MSVCRT___utimbuf32 *t)
{
    if (t)
    {
        struct MSVCRT___utimbuf64 t64;
        t64.actime = t->actime;
        t64.modtime = t->modtime;
        return _futime64( fd, &t64 );
    }
    else
        return _futime64( fd, NULL );
}

/*********************************************************************
 *		_futime (MSVCRT.@)
 */
#ifdef _WIN64
int CDECL _futime(int fd, struct MSVCRT___utimbuf64 *t)
{
    return _futime64( fd, t );
}
#else
int CDECL _futime(int fd, struct MSVCRT___utimbuf32 *t)
{
    return _futime32( fd, t );
}
#endif

/*********************************************************************
 *		_get_osfhandle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL MSVCRT__get_osfhandle(int fd)
{
  HANDLE hand = msvcrt_fdtoh(fd);
  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  return (MSVCRT_intptr_t)hand;
}

/*********************************************************************
 *		_isatty (MSVCRT.@)
 */
int CDECL MSVCRT__isatty(int fd)
{
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return 0;

  return GetFileType(hand) == FILE_TYPE_CHAR? 1 : 0;
}

/*********************************************************************
 *		_mktemp (MSVCRT.@)
 */
char * CDECL MSVCRT__mktemp(char *pattern)
{
  int numX = 0;
  char *retVal = pattern;
  int id;
  char letter = 'a';

  while(*pattern)
    numX = (*pattern++ == 'X')? numX + 1 : 0;
  if (numX < 5)
    return NULL;
  pattern--;
  id = GetCurrentProcessId();
  numX = 6;
  while(numX--)
  {
    int tempNum = id / 10;
    *pattern-- = id - (tempNum * 10) + '0';
    id = tempNum;
  }
  pattern++;
  do
  {
    *pattern = letter++;
    if (GetFileAttributesA(retVal) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      return retVal;
  } while(letter <= 'z');
  return NULL;
}

/*********************************************************************
 *		_wmktemp (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT__wmktemp(MSVCRT_wchar_t *pattern)
{
  int numX = 0;
  MSVCRT_wchar_t *retVal = pattern;
  int id;
  MSVCRT_wchar_t letter = 'a';

  while(*pattern)
    numX = (*pattern++ == 'X')? numX + 1 : 0;
  if (numX < 5)
    return NULL;
  pattern--;
  id = GetCurrentProcessId();
  numX = 6;
  while(numX--)
  {
    int tempNum = id / 10;
    *pattern-- = id - (tempNum * 10) + '0';
    id = tempNum;
  }
  pattern++;
  do
  {
    if (GetFileAttributesW(retVal) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      return retVal;
    *pattern = letter++;
  } while(letter != '|');
  return NULL;
}

static unsigned split_oflags(unsigned oflags)
{
    int         wxflags = 0;
    unsigned unsupp; /* until we support everything */

    if (oflags & MSVCRT__O_APPEND)              wxflags |= WX_APPEND;
    if (oflags & MSVCRT__O_BINARY)              {/* Nothing to do */}
    else if (oflags & MSVCRT__O_TEXT)           wxflags |= WX_TEXT;
    else if (*__p__fmode() & MSVCRT__O_BINARY)  {/* Nothing to do */}
    else                                        wxflags |= WX_TEXT; /* default to TEXT*/
    if (oflags & MSVCRT__O_NOINHERIT)           wxflags |= WX_DONTINHERIT;

    if ((unsupp = oflags & ~(
                    MSVCRT__O_BINARY|MSVCRT__O_TEXT|MSVCRT__O_APPEND|
                    MSVCRT__O_TRUNC|MSVCRT__O_EXCL|MSVCRT__O_CREAT|
                    MSVCRT__O_RDWR|MSVCRT__O_WRONLY|MSVCRT__O_TEMPORARY|
                    MSVCRT__O_NOINHERIT|
                    MSVCRT__O_SEQUENTIAL|MSVCRT__O_RANDOM|MSVCRT__O_SHORT_LIVED
                    )))
        ERR(":unsupported oflags 0x%04x\n",unsupp);

    return wxflags;
}

/*********************************************************************
 *              _pipe (MSVCRT.@)
 */
int CDECL MSVCRT__pipe(int *pfds, unsigned int psize, int textmode)
{
  int ret = -1;
  SECURITY_ATTRIBUTES sa;
  HANDLE readHandle, writeHandle;

  if (!pfds)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = !(textmode & MSVCRT__O_NOINHERIT);
  sa.lpSecurityDescriptor = NULL;
  if (CreatePipe(&readHandle, &writeHandle, &sa, psize))
  {
    unsigned int wxflags = split_oflags(textmode);
    int fd;

    LOCK_FILES();
    fd = msvcrt_alloc_fd(readHandle, wxflags);
    if (fd != -1)
    {
      pfds[0] = fd;
      fd = msvcrt_alloc_fd(writeHandle, wxflags);
      if (fd != -1)
      {
        pfds[1] = fd;
        ret = 0;
      }
      else
      {
        MSVCRT__close(pfds[0]);
        CloseHandle(writeHandle);
        *MSVCRT__errno() = MSVCRT_EMFILE;
      }
    }
    else
    {
      CloseHandle(readHandle);
      CloseHandle(writeHandle);
      *MSVCRT__errno() = MSVCRT_EMFILE;
    }
    UNLOCK_FILES();
  }
  else
    msvcrt_set_errno(GetLastError());

  return ret;
}

/*********************************************************************
 *              _sopen_s (MSVCRT.@)
 */
int CDECL MSVCRT__sopen_s( int *fd, const char *path, int oflags, int shflags, int pmode )
{
  DWORD access = 0, creation = 0, attrib;
  DWORD sharing;
  int wxflag;
  HANDLE hand;
  SECURITY_ATTRIBUTES sa;

  TRACE("fd*: %p file: (%s) oflags: 0x%04x shflags: 0x%04x pmode: 0x%04x\n",
        fd, path, oflags, shflags, pmode);

  if (!MSVCRT_CHECK_PMT( fd != NULL )) return MSVCRT_EINVAL;

  *fd = -1;
  wxflag = split_oflags(oflags);
  switch (oflags & (MSVCRT__O_RDONLY | MSVCRT__O_WRONLY | MSVCRT__O_RDWR))
  {
  case MSVCRT__O_RDONLY: access |= GENERIC_READ; break;
  case MSVCRT__O_WRONLY: access |= GENERIC_WRITE; break;
  case MSVCRT__O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & MSVCRT__O_CREAT)
  {
    if(pmode & ~(MSVCRT__S_IREAD | MSVCRT__S_IWRITE))
      FIXME(": pmode 0x%04x ignored\n", pmode);
    else
      WARN(": pmode 0x%04x ignored\n", pmode);

    if (oflags & MSVCRT__O_EXCL)
      creation = CREATE_NEW;
    else if (oflags & MSVCRT__O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  }
  else  /* no MSVCRT__O_CREAT */
  {
    if (oflags & MSVCRT__O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }
  
  switch( shflags )
  {
    case MSVCRT__SH_DENYRW:
      sharing = 0L;
      break;
    case MSVCRT__SH_DENYWR:
      sharing = FILE_SHARE_READ;
      break;
    case MSVCRT__SH_DENYRD:
      sharing = FILE_SHARE_WRITE;
      break;
    case MSVCRT__SH_DENYNO:
      sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
    default:
      ERR( "Unhandled shflags 0x%x\n", shflags );
      return MSVCRT_EINVAL;
  }
  attrib = FILE_ATTRIBUTE_NORMAL;

  if (oflags & MSVCRT__O_TEMPORARY)
  {
      attrib |= FILE_FLAG_DELETE_ON_CLOSE;
      access |= DELETE;
      sharing |= FILE_SHARE_DELETE;
  }

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = (oflags & MSVCRT__O_NOINHERIT) ? FALSE : TRUE;

  hand = CreateFileA(path, access, sharing, &sa, creation, attrib, 0);
  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%d)\n", GetLastError());
    msvcrt_set_errno(GetLastError());
    return *MSVCRT__errno();
  }

  *fd = msvcrt_alloc_fd(hand, wxflag);

  TRACE(":fd (%d) handle (%p)\n", *fd, hand);
  return 0;
}

/*********************************************************************
 *              _sopen (MSVCRT.@)
 */
int CDECL MSVCRT__sopen( const char *path, int oflags, int shflags, ... )
{
  int pmode;
  int fd;

  if (oflags & MSVCRT__O_CREAT)
  {
    __ms_va_list ap;

    __ms_va_start(ap, shflags);
    pmode = va_arg(ap, int);
    __ms_va_end(ap);
  }
  else
    pmode = 0;

  MSVCRT__sopen_s(&fd, path, oflags, shflags, pmode);
  return fd;
}

/*********************************************************************
 *              _wsopen_s (MSVCRT.@)
 */
int CDECL MSVCRT__wsopen_s( int *fd, const MSVCRT_wchar_t* path, int oflags, int shflags, int pmode )
{
  DWORD access = 0, creation = 0, attrib;
  SECURITY_ATTRIBUTES sa;
  DWORD sharing;
  int wxflag;
  HANDLE hand;

  TRACE("fd*: %p :file (%s) oflags: 0x%04x shflags: 0x%04x pmode: 0x%04x\n",
        fd, debugstr_w(path), oflags, shflags, pmode);

  if (!MSVCRT_CHECK_PMT( fd != NULL )) return MSVCRT_EINVAL;

  *fd = -1;
  wxflag = split_oflags(oflags);
  switch (oflags & (MSVCRT__O_RDONLY | MSVCRT__O_WRONLY | MSVCRT__O_RDWR))
  {
  case MSVCRT__O_RDONLY: access |= GENERIC_READ; break;
  case MSVCRT__O_WRONLY: access |= GENERIC_WRITE; break;
  case MSVCRT__O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & MSVCRT__O_CREAT)
  {
    if(pmode & ~(MSVCRT__S_IREAD | MSVCRT__S_IWRITE))
      FIXME(": pmode 0x%04x ignored\n", pmode);
    else
      WARN(": pmode 0x%04x ignored\n", pmode);

    if (oflags & MSVCRT__O_EXCL)
      creation = CREATE_NEW;
    else if (oflags & MSVCRT__O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  }
  else  /* no MSVCRT__O_CREAT */
  {
    if (oflags & MSVCRT__O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }

  switch( shflags )
  {
    case MSVCRT__SH_DENYRW:
      sharing = 0L;
      break;
    case MSVCRT__SH_DENYWR:
      sharing = FILE_SHARE_READ;
      break;
    case MSVCRT__SH_DENYRD:
      sharing = FILE_SHARE_WRITE;
      break;
    case MSVCRT__SH_DENYNO:
      sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
    default:
      ERR( "Unhandled shflags 0x%x\n", shflags );
      return MSVCRT_EINVAL;
  }
  attrib = FILE_ATTRIBUTE_NORMAL;

  if (oflags & MSVCRT__O_TEMPORARY)
  {
      attrib |= FILE_FLAG_DELETE_ON_CLOSE;
      access |= DELETE;
      sharing |= FILE_SHARE_DELETE;
  }

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = (oflags & MSVCRT__O_NOINHERIT) ? FALSE : TRUE;

  hand = CreateFileW(path, access, sharing, &sa, creation, attrib, 0);

  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%d)\n",GetLastError());
    msvcrt_set_errno(GetLastError());
    msvcrt_set_errno(GetLastError());
    return *MSVCRT__errno();
  }

  *fd = msvcrt_alloc_fd(hand, wxflag);

  TRACE(":fd (%d) handle (%p)\n", *fd, hand);
  return 0;
}

/*********************************************************************
 *              _wsopen (MSVCRT.@)
 */
int CDECL MSVCRT__wsopen( const MSVCRT_wchar_t *path, int oflags, int shflags, ... )
{
  int pmode;
  int fd;

  if (oflags & MSVCRT__O_CREAT)
  {
    __ms_va_list ap;

    __ms_va_start(ap, shflags);
    pmode = va_arg(ap, int);
    __ms_va_end(ap);
  }
  else
    pmode = 0;

  MSVCRT__wsopen_s(&fd, path, oflags, shflags, pmode);
  return fd;
}

/*********************************************************************
 *              _open (MSVCRT.@)
 */
int CDECL MSVCRT__open( const char *path, int flags, ... )
{
  __ms_va_list ap;

  if (flags & MSVCRT__O_CREAT)
  {
    int pmode;
    __ms_va_start(ap, flags);
    pmode = va_arg(ap, int);
    __ms_va_end(ap);
    return MSVCRT__sopen( path, flags, MSVCRT__SH_DENYNO, pmode );
  }
  else
    return MSVCRT__sopen( path, flags, MSVCRT__SH_DENYNO);
}

/*********************************************************************
 *              _wopen (MSVCRT.@)
 */
int CDECL MSVCRT__wopen(const MSVCRT_wchar_t *path,int flags,...)
{
  __ms_va_list ap;

  if (flags & MSVCRT__O_CREAT)
  {
    int pmode;
    __ms_va_start(ap, flags);
    pmode = va_arg(ap, int);
    __ms_va_end(ap);
    return MSVCRT__wsopen( path, flags, MSVCRT__SH_DENYNO, pmode );
  }
  else
    return MSVCRT__wsopen( path, flags, MSVCRT__SH_DENYNO);
}

/*********************************************************************
 *		_creat (MSVCRT.@)
 */
int CDECL MSVCRT__creat(const char *path, int flags)
{
  int usedFlags = (flags & MSVCRT__O_TEXT)| MSVCRT__O_CREAT| MSVCRT__O_WRONLY| MSVCRT__O_TRUNC;
  return MSVCRT__open(path, usedFlags);
}

/*********************************************************************
 *		_wcreat (MSVCRT.@)
 */
int CDECL MSVCRT__wcreat(const MSVCRT_wchar_t *path, int flags)
{
  int usedFlags = (flags & MSVCRT__O_TEXT)| MSVCRT__O_CREAT| MSVCRT__O_WRONLY| MSVCRT__O_TRUNC;
  return MSVCRT__wopen(path, usedFlags);
}

/*********************************************************************
 *		_open_osfhandle (MSVCRT.@)
 */
int CDECL MSVCRT__open_osfhandle(MSVCRT_intptr_t handle, int oflags)
{
  int fd;

  /* MSVCRT__O_RDONLY (0) always matches, so set the read flag
   * MFC's CStdioFile clears O_RDONLY (0)! if it wants to write to the
   * file, so set the write flag. It also only sets MSVCRT__O_TEXT if it wants
   * text - it never sets MSVCRT__O_BINARY.
   */
  /* don't let split_oflags() decide the mode if no mode is passed */
  if (!(oflags & (MSVCRT__O_BINARY | MSVCRT__O_TEXT)))
      oflags |= MSVCRT__O_BINARY;

  fd = msvcrt_alloc_fd((HANDLE)handle, split_oflags(oflags));
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n", handle, fd, oflags);
  return fd;
}

/*********************************************************************
 *		_rmtmp (MSVCRT.@)
 */
int CDECL MSVCRT__rmtmp(void)
{
  int num_removed = 0, i;
  MSVCRT_FILE *file;

  LOCK_FILES();
  for (i = 3; i < MSVCRT_stream_idx; i++) {
    file = msvcrt_get_file(i);

    if (file->_tmpfname)
    {
      MSVCRT_fclose(file);
      num_removed++;
    }
  }
  UNLOCK_FILES();

  if (num_removed)
    TRACE(":removed (%d) temp files\n",num_removed);
  return num_removed;
}

/*********************************************************************
 * (internal) read_i
 *
 * When reading \r as last character in text mode, read() positions
 * the file pointer on the \r character while getc() goes on to
 * the following \n
 */
static int read_i(int fd, void *buf, unsigned int count)
{
  DWORD num_read;
  char *bufstart = buf;
  HANDLE hand = msvcrt_fdtoh(fd);
  ioinfo *fdinfo = msvcrt_get_ioinfo(fd);

  if (count == 0)
    return 0;

  if (fdinfo->wxflag & WX_READEOF) {
     fdinfo->wxflag |= WX_ATEOF;
     TRACE("already at EOF, returning 0\n");
     return 0;
  }
  /* Don't trace small reads, it gets *very* annoying */
  if (count > 4)
    TRACE(":fd (%d) handle (%p) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Reading single bytes in O_TEXT mode makes things slow
   * So read big chunks
   */
    if (ReadFile(hand, bufstart, count, &num_read, NULL))
    {
        if (count != 0 && num_read == 0)
        {
            fdinfo->wxflag |= (WX_ATEOF|WX_READEOF);
            TRACE(":EOF %s\n",debugstr_an(buf,num_read));
        }
        else if (fdinfo->wxflag & WX_TEXT)
        {
            DWORD i, j;
            if (bufstart[num_read-1] == '\r')
            {
                if(count == 1)
                {
                    fdinfo->wxflag  &=  ~WX_READCR;
                    ReadFile(hand, bufstart, 1, &num_read, NULL);
                }
                else
                {
                    fdinfo->wxflag  |= WX_READCR;
                    num_read--;
                }
            }
	    else
	      fdinfo->wxflag  &=  ~WX_READCR;
            for (i=0, j=0; i<num_read; i++)
            {
                /* in text mode, a ctrl-z signals EOF */
                if (bufstart[i] == 0x1a)
                {
                    fdinfo->wxflag |= (WX_ATEOF|WX_READEOF);
                    TRACE(":^Z EOF %s\n",debugstr_an(buf,num_read));
                    break;
                }
                /* in text mode, strip \r if followed by \n.
                 * BUG: should save state across calls somehow, so CR LF that
                 * straddles buffer boundary gets recognized properly?
                 */
		if ((bufstart[i] != '\r')
                ||  ((i+1) < num_read && bufstart[i+1] != '\n'))
		    bufstart[j++] = bufstart[i];
            }
            num_read = j;
        }
    }
    else
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            TRACE(":end-of-pipe\n");
            fdinfo->wxflag |= (WX_ATEOF|WX_READEOF);
            return 0;
        }
        else
        {
            TRACE(":failed-last error (%d)\n",GetLastError());
            return -1;
        }
    }

  if (count > 4)
      TRACE("(%u), %s\n",num_read,debugstr_an(buf, num_read));
  return num_read;
}

/*********************************************************************
 *		_read (MSVCRT.@)
 */
int CDECL MSVCRT__read(int fd, void *buf, unsigned int count)
{
  int num_read;
  num_read = read_i(fd, buf, count);
  return num_read;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int CDECL MSVCRT__setmode(int fd,int mode)
{
  int ret = msvcrt_get_ioinfo(fd)->wxflag & WX_TEXT ? MSVCRT__O_TEXT : MSVCRT__O_BINARY;
  if (mode & (~(MSVCRT__O_TEXT|MSVCRT__O_BINARY)))
    FIXME("fd (%d) mode (0x%08x) unknown\n",fd,mode);
  if ((mode & MSVCRT__O_TEXT) == MSVCRT__O_TEXT)
    msvcrt_get_ioinfo(fd)->wxflag |= WX_TEXT;
  else
    msvcrt_get_ioinfo(fd)->wxflag &= ~WX_TEXT;
  return ret;
}

/*********************************************************************
 *		_stat64 (MSVCRT.@)
 */
int CDECL MSVCRT_stat64(const char* path, struct MSVCRT__stat64 * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct MSVCRT__stat64));

  /* FIXME: rdev isn't drive num, despite what the docs say-what is it?
     Bon 011120: This FIXME seems incorrect
                 Also a letter as first char isn't enough to be classified
		 as a drive letter
  */
  if (isalpha(*path)&& (*(path+1)==':'))
    buf->st_dev = buf->st_rdev = toupper(*path) - 'A'; /* drive num */
  else
    buf->st_dev = buf->st_rdev = MSVCRT__getdrive() - 1;

  plen = strlen(path);

  /* Dir, or regular file? */
  if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
      (path[plen-1] == '\\'))
    mode |= (MSVCRT__S_IFDIR | ALL_S_IEXEC);
  else
  {
    mode |= MSVCRT__S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
      unsigned int ext = tolower(path[plen-1]) | (tolower(path[plen-2]) << 8) |
                                 (tolower(path[plen-3]) << 16);
      if (ext == EXE || ext == BAT || ext == CMD || ext == COM)
          mode |= ALL_S_IEXEC;
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= ALL_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
  buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("%d %d 0x%08x%08x %d %d %d\n", buf->st_mode,buf->st_nlink,
        (int)(buf->st_size >> 32),(int)buf->st_size,
        (int)buf->st_atime,(int)buf->st_mtime,(int)buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_stati64 (MSVCRT.@)
 */
int CDECL MSVCRT_stati64(const char* path, struct MSVCRT__stati64 * buf)
{
  int ret;
  struct MSVCRT__stat64 buf64;

  ret = MSVCRT_stat64(path, &buf64);
  if (!ret)
    msvcrt_stat64_to_stati64(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_stat (MSVCRT.@)
 */
int CDECL MSVCRT_stat(const char* path, struct MSVCRT__stat * buf)
{ int ret;
  struct MSVCRT__stat64 buf64;

  ret = MSVCRT_stat64( path, &buf64);
  if (!ret)
      msvcrt_stat64_to_stat(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_wstat64 (MSVCRT.@)
 */
int CDECL MSVCRT__wstat64(const MSVCRT_wchar_t* path, struct MSVCRT__stat64 * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",debugstr_w(path),buf);

  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct MSVCRT__stat64));

  /* FIXME: rdev isn't drive num, despite what the docs says-what is it? */
  if (MSVCRT_iswalpha(*path))
    buf->st_dev = buf->st_rdev = toupperW(*path - 'A'); /* drive num */
  else
    buf->st_dev = buf->st_rdev = MSVCRT__getdrive() - 1;

  plen = strlenW(path);

  /* Dir, or regular file? */
  if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
      (path[plen-1] == '\\'))
    mode |= (MSVCRT__S_IFDIR | ALL_S_IEXEC);
  else
  {
    mode |= MSVCRT__S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
      ULONGLONG ext = tolowerW(path[plen-1]) | (tolowerW(path[plen-2]) << 16) |
                               ((ULONGLONG)tolowerW(path[plen-3]) << 32);
      if (ext == WCEXE || ext == WCBAT || ext == WCCMD || ext == WCCOM)
        mode |= ALL_S_IEXEC;
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= ALL_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
  buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("%d %d 0x%08x%08x %d %d %d\n", buf->st_mode,buf->st_nlink,
        (int)(buf->st_size >> 32),(int)buf->st_size,
        (int)buf->st_atime,(int)buf->st_mtime,(int)buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_wstati64 (MSVCRT.@)
 */
int CDECL MSVCRT__wstati64(const MSVCRT_wchar_t* path, struct MSVCRT__stati64 * buf)
{
  int ret;
  struct MSVCRT__stat64 buf64;

  ret = MSVCRT__wstat64(path, &buf64);
  if (!ret)
    msvcrt_stat64_to_stati64(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_wstat (MSVCRT.@)
 */
int CDECL MSVCRT__wstat(const MSVCRT_wchar_t* path, struct MSVCRT__stat * buf)
{
  int ret;
  struct MSVCRT__stat64 buf64;

  ret = MSVCRT__wstat64( path, &buf64 );
  if (!ret) msvcrt_stat64_to_stat(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_tell (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT__tell(int fd)
{
  return MSVCRT__lseek(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_telli64 (MSVCRT.@)
 */
__int64 CDECL _telli64(int fd)
{
  return MSVCRT__lseeki64(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_tempnam (MSVCRT.@)
 */
char * CDECL MSVCRT__tempnam(const char *dir, const char *prefix)
{
  char tmpbuf[MAX_PATH];
  const char *tmp_dir = MSVCRT_getenv("TMP");

  if (tmp_dir) dir = tmp_dir;

  TRACE("dir (%s) prefix (%s)\n",dir,prefix);
  if (GetTempFileNameA(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",tmpbuf);
    DeleteFileA(tmpbuf);
    return MSVCRT__strdup(tmpbuf);
  }
  TRACE("failed (%d)\n",GetLastError());
  return NULL;
}

/*********************************************************************
 *		_wtempnam (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT__wtempnam(const MSVCRT_wchar_t *dir, const MSVCRT_wchar_t *prefix)
{
  static const MSVCRT_wchar_t tmpW[] = {'T','M','P',0};
  MSVCRT_wchar_t tmpbuf[MAX_PATH];
  const MSVCRT_wchar_t *tmp_dir = MSVCRT__wgetenv(tmpW);

  if (tmp_dir) dir = tmp_dir;

  TRACE("dir (%s) prefix (%s)\n",debugstr_w(dir),debugstr_w(prefix));
  if (GetTempFileNameW(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",debugstr_w(tmpbuf));
    DeleteFileW(tmpbuf);
    return MSVCRT__wcsdup(tmpbuf);
  }
  TRACE("failed (%d)\n",GetLastError());
  return NULL;
}

/*********************************************************************
 *		_umask (MSVCRT.@)
 */
int CDECL MSVCRT__umask(int umask)
{
  int old_umask = MSVCRT_umask;
  TRACE("(%d)\n",umask);
  MSVCRT_umask = umask;
  return old_umask;
}

/*********************************************************************
 *		_utime64 (MSVCRT.@)
 */
int CDECL _utime64(const char* path, struct MSVCRT___utimbuf64 *t)
{
  int fd = MSVCRT__open(path, MSVCRT__O_WRONLY | MSVCRT__O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime64(fd, t);
    MSVCRT__close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_utime32 (MSVCRT.@)
 */
int CDECL _utime32(const char* path, struct MSVCRT___utimbuf32 *t)
{
    if (t)
    {
        struct MSVCRT___utimbuf64 t64;
        t64.actime = t->actime;
        t64.modtime = t->modtime;
        return _utime64( path, &t64 );
    }
    else
        return _utime64( path, NULL );
}

/*********************************************************************
 *		_utime (MSVCRT.@)
 */
#ifdef _WIN64
int CDECL _utime(const char* path, struct MSVCRT___utimbuf64 *t)
{
    return _utime64( path, t );
}
#else
int CDECL _utime(const char* path, struct MSVCRT___utimbuf32 *t)
{
    return _utime32( path, t );
}
#endif

/*********************************************************************
 *		_wutime64 (MSVCRT.@)
 */
int CDECL _wutime64(const MSVCRT_wchar_t* path, struct MSVCRT___utimbuf64 *t)
{
  int fd = MSVCRT__wopen(path, MSVCRT__O_WRONLY | MSVCRT__O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime64(fd, t);
    MSVCRT__close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_wutime32 (MSVCRT.@)
 */
int CDECL _wutime32(const MSVCRT_wchar_t* path, struct MSVCRT___utimbuf32 *t)
{
    if (t)
    {
        struct MSVCRT___utimbuf64 t64;
        t64.actime = t->actime;
        t64.modtime = t->modtime;
        return _wutime64( path, &t64 );
    }
    else
        return _wutime64( path, NULL );
}

/*********************************************************************
 *		_wutime (MSVCRT.@)
 */
#ifdef _WIN64
int CDECL _wutime(const MSVCRT_wchar_t* path, struct MSVCRT___utimbuf64 *t)
{
    return _wutime64( path, t );
}
#else
int CDECL _wutime(const MSVCRT_wchar_t* path, struct MSVCRT___utimbuf32 *t)
{
    return _wutime32( path, t );
}
#endif

/*********************************************************************
 *		_write (MSVCRT.@)
 */
int CDECL MSVCRT__write(int fd, const void* buf, unsigned int count)
{
  DWORD num_written;
  HANDLE hand = msvcrt_fdtoh(fd);

  /* Don't trace small writes, it gets *very* annoying */
#if 0
  if (count > 32)
    TRACE(":fd (%d) handle (%d) buf (%p) len (%d)\n",fd,hand,buf,count);
#endif
  if (hand == INVALID_HANDLE_VALUE)
    {
      *MSVCRT__errno() = MSVCRT_EBADF;
      return -1;
    }

  /* If appending, go to EOF */
  if (msvcrt_get_ioinfo(fd)->wxflag & WX_APPEND)
    MSVCRT__lseek(fd, 0, FILE_END);

  if (!(msvcrt_get_ioinfo(fd)->wxflag & WX_TEXT))
    {
      if (WriteFile(hand, buf, count, &num_written, NULL)
	  &&  (num_written == count))
	return num_written;
      TRACE("WriteFile (fd %d, hand %p) failed-last error (%d)\n", fd,
       hand, GetLastError());
      *MSVCRT__errno() = MSVCRT_ENOSPC;
    }
  else
  {
      unsigned int i, j, nr_lf;
      char *p = NULL;
      const char *q;
      const char *s = buf, *buf_start = buf;
      /* find number of \n ( without preceding \r ) */
      for ( nr_lf=0,i = 0; i <count; i++)
      {
          if (s[i]== '\n')
          {
              nr_lf++;
              /*if ((i >1) && (s[i-1] == '\r'))	nr_lf--; */
          }
      }
      if (nr_lf)
      {
          if ((q = p = MSVCRT_malloc(count + nr_lf)))
          {
              for (s = buf, i = 0, j = 0; i < count; i++)
              {
                  if (s[i]== '\n')
                  {
                      p[j++] = '\r';
                      /*if ((i >1) && (s[i-1] == '\r'))j--;*/
                  }
                  p[j++] = s[i];
              }
          }
          else
          {
              FIXME("Malloc failed\n");
              nr_lf =0;
              q = buf;
          }
      }
      else
          q = buf;

      if ((WriteFile(hand, q, count+nr_lf, &num_written, NULL) == 0 ) || (num_written != count+nr_lf))
      {
          TRACE("WriteFile (fd %d, hand %p) failed-last error (%d), num_written %d\n",
           fd, hand, GetLastError(), num_written);
          *MSVCRT__errno() = MSVCRT_ENOSPC;
          if(nr_lf)
              MSVCRT_free(p);
          return s - buf_start;
      }
      else
      {
          if(nr_lf)
              MSVCRT_free(p);
          return count;
      }
  }
  return -1;
}

/*********************************************************************
 *		_putw (MSVCRT.@)
 */
int CDECL MSVCRT__putw(int val, MSVCRT_FILE* file)
{
  int len;

  MSVCRT__lock_file(file);
  len = MSVCRT__write(file->_file, &val, sizeof(val));
  if (len == sizeof(val)) {
    MSVCRT__unlock_file(file);
    return val;
  }

  file->_flag |= MSVCRT__IOERR;
  MSVCRT__unlock_file(file);
  return MSVCRT_EOF;
}

/*********************************************************************
 *		fclose (MSVCRT.@)
 */
int CDECL MSVCRT_fclose(MSVCRT_FILE* file)
{
  int r, flag;

  MSVCRT__lock_file(file);
  flag = file->_flag;
  MSVCRT_free(file->_tmpfname);
  file->_tmpfname = NULL;
  /* flush stdio buffers */
  if(file->_flag & MSVCRT__IOWRT)
      MSVCRT_fflush(file);
  if(file->_flag & MSVCRT__IOMYBUF)
      MSVCRT_free(file->_base);

  r=MSVCRT__close(file->_file);

  file->_flag = 0;
  MSVCRT__unlock_file(file);

  return ((r == -1) || (flag & MSVCRT__IOERR) ? MSVCRT_EOF : 0);
}

/*********************************************************************
 *		feof (MSVCRT.@)
 */
int CDECL MSVCRT_feof(MSVCRT_FILE* file)
{
    return file->_flag & MSVCRT__IOEOF;
}

/*********************************************************************
 *		ferror (MSVCRT.@)
 */
int CDECL MSVCRT_ferror(MSVCRT_FILE* file)
{
    return file->_flag & MSVCRT__IOERR;
}

/*********************************************************************
 *		_filbuf (MSVCRT.@)
 */
int CDECL MSVCRT__filbuf(MSVCRT_FILE* file)
{
    unsigned char c;
    MSVCRT__lock_file(file);

    /* Allocate buffer if needed */
    if(file->_bufsiz == 0 && !(file->_flag & MSVCRT__IONBF))
        msvcrt_alloc_buffer(file);

    if(!(file->_flag & MSVCRT__IOREAD)) {
        if(file->_flag & MSVCRT__IORW)
            file->_flag |= MSVCRT__IOREAD;
        else {
            MSVCRT__unlock_file(file);
            return MSVCRT_EOF;
        }
    }

    if(file->_flag & MSVCRT__IONBF) {
        int r;
        if ((r = read_i(file->_file,&c,1)) != 1) {
            file->_flag |= (r == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
            MSVCRT__unlock_file(file);
            return MSVCRT_EOF;
        }

        MSVCRT__unlock_file(file);
        return c;
    } else {
        file->_cnt = read_i(file->_file, file->_base, file->_bufsiz);
        if(file->_cnt<=0) {
            file->_flag |= (file->_cnt == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
            file->_cnt = 0;
            MSVCRT__unlock_file(file);
            return MSVCRT_EOF;
        }

        file->_cnt--;
        file->_ptr = file->_base+1;
        c = *(unsigned char *)file->_base;
        MSVCRT__unlock_file(file);
        return c;
    }
}

/*********************************************************************
 *		fgetc (MSVCRT.@)
 */
int CDECL MSVCRT_fgetc(MSVCRT_FILE* file)
{
  unsigned char *i;
  unsigned int j;

  MSVCRT__lock_file(file);
  if (file->_cnt>0) {
    file->_cnt--;
    i = (unsigned char *)file->_ptr++;
    j = *i;
  } else
    j = MSVCRT__filbuf(file);

  MSVCRT__unlock_file(file);
  return j;
}

/*********************************************************************
 *		_fgetchar (MSVCRT.@)
 */
int CDECL MSVCRT__fgetchar(void)
{
  return MSVCRT_fgetc(MSVCRT_stdin);
}

/*********************************************************************
 *		fgets (MSVCRT.@)
 */
char * CDECL MSVCRT_fgets(char *s, int size, MSVCRT_FILE* file)
{
  int    cc = MSVCRT_EOF;
  char * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
	file,file->_file,s,size);

  MSVCRT__lock_file(file);

  while ((size >1) && (cc = MSVCRT_fgetc(file)) != MSVCRT_EOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == MSVCRT_EOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    MSVCRT__unlock_file(file);
    return NULL;
  }
  if ((cc != MSVCRT_EOF) && (size > 1))
    *s++ = cc;
  *s = '\0';
  TRACE(":got %s\n", debugstr_a(buf_start));
  MSVCRT__unlock_file(file);
  return buf_start;
}

/*********************************************************************
 *		fgetwc (MSVCRT.@)
 *
 * In MSVCRT__O_TEXT mode, multibyte characters are read from the file, dropping
 * the CR from CR/LF combinations
 */
MSVCRT_wint_t CDECL MSVCRT_fgetwc(MSVCRT_FILE* file)
{
  int c;

  MSVCRT__lock_file(file);
  if (!(msvcrt_get_ioinfo(file->_file)->wxflag & WX_TEXT))
    {
      MSVCRT_wchar_t wc;
      unsigned int i;
      int j;
      char *chp, *wcp;
      wcp = (char *)&wc;
      for(i=0; i<sizeof(wc); i++)
      {
        if (file->_cnt>0) 
        {
          file->_cnt--;
          chp = file->_ptr++;
          wcp[i] = *chp;
        } 
        else
        {
          j = MSVCRT__filbuf(file);
          if(file->_cnt<=0)
          {
            file->_flag |= (file->_cnt == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
            file->_cnt = 0;

            MSVCRT__unlock_file(file);
            return MSVCRT_WEOF;
          }
          wcp[i] = j;
        }
      }

      MSVCRT__unlock_file(file);
      return wc;
    }
    
  c = MSVCRT_fgetc(file);
  if ((get_locinfo()->mb_cur_max > 1) && MSVCRT_isleadbyte(c))
    {
      FIXME("Treat Multibyte characters\n");
    }

  MSVCRT__unlock_file(file);
  if (c == MSVCRT_EOF)
    return MSVCRT_WEOF;
  else
    return (MSVCRT_wint_t)c;
}

/*********************************************************************
 *		_getw (MSVCRT.@)
 */
int CDECL MSVCRT__getw(MSVCRT_FILE* file)
{
  char *ch;
  int i, k;
  unsigned int j;
  ch = (char *)&i;

  MSVCRT__lock_file(file);
  for (j=0; j<sizeof(int); j++) {
    k = MSVCRT_fgetc(file);
    if (k == MSVCRT_EOF) {
      file->_flag |= MSVCRT__IOEOF;
      MSVCRT__unlock_file(file);
      return EOF;
    }
    ch[j] = k;
  }

  MSVCRT__unlock_file(file);
  return i;
}

/*********************************************************************
 *		getwc (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT_getwc(MSVCRT_FILE* file)
{
  return MSVCRT_fgetwc(file);
}

/*********************************************************************
 *		_fgetwchar (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT__fgetwchar(void)
{
  return MSVCRT_fgetwc(MSVCRT_stdin);
}

/*********************************************************************
 *		getwchar (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT_getwchar(void)
{
  return MSVCRT__fgetwchar();
}

/*********************************************************************
 *              fgetws (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT_fgetws(MSVCRT_wchar_t *s, int size, MSVCRT_FILE* file)
{
  int    cc = MSVCRT_WEOF;
  MSVCRT_wchar_t * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
        file,file->_file,s,size);

  MSVCRT__lock_file(file);

  while ((size >1) && (cc = MSVCRT_fgetwc(file)) != MSVCRT_WEOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == MSVCRT_WEOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    MSVCRT__unlock_file(file);
    return NULL;
  }
  if ((cc != MSVCRT_WEOF) && (size > 1))
    *s++ = cc;
  *s = 0;
  TRACE(":got %s\n", debugstr_w(buf_start));
  MSVCRT__unlock_file(file);
  return buf_start;
}

/*********************************************************************
 *		fwrite (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_fwrite(const void *ptr, MSVCRT_size_t size, MSVCRT_size_t nmemb, MSVCRT_FILE* file)
{
    MSVCRT_size_t wrcnt=size * nmemb;
    int written = 0;
    if (size == 0)
        return 0;

    MSVCRT__lock_file(file);
    if(file->_cnt) {
        int pcnt=(file->_cnt>wrcnt)? wrcnt: file->_cnt;
        memcpy(file->_ptr, ptr, pcnt);
        file->_cnt -= pcnt;
        file->_ptr += pcnt;
        written = pcnt;
        wrcnt -= pcnt;
        ptr = (const char*)ptr + pcnt;
    } else if(!(file->_flag & MSVCRT__IOWRT)) {
        if(file->_flag & MSVCRT__IORW) {
            file->_flag |= MSVCRT__IOWRT;
        } else {
            MSVCRT__unlock_file(file);
            return 0;
        }
    }
    if(wrcnt) {
        /* Flush buffer */
        int res=msvcrt_flush_buffer(file);
        if(!res) {
            int pwritten = MSVCRT__write(file->_file, ptr, wrcnt);
            if (pwritten <= 0)
            {
                file->_flag |= MSVCRT__IOERR;
                pwritten=0;
            }
            written += pwritten;
        }
    }

    MSVCRT__unlock_file(file);
    return written / size;
}

/*********************************************************************
 *		fputwc (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT_fputwc(MSVCRT_wint_t wc, MSVCRT_FILE* file)
{
  MSVCRT_wchar_t mwc=wc;
  if (MSVCRT_fwrite( &mwc, sizeof(mwc), 1, file) != 1)
    return MSVCRT_WEOF;
  return wc;
}

/*********************************************************************
 *		_fputwchar (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT__fputwchar(MSVCRT_wint_t wc)
{
  return MSVCRT_fputwc(wc, MSVCRT_stdout);
}

/*********************************************************************
 *		_wfsopen (MSVCRT.@)
 */
MSVCRT_FILE * CDECL MSVCRT__wfsopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode, int share)
{
  MSVCRT_FILE* file;
  int open_flags, stream_flags, fd;

  TRACE("(%s,%s)\n", debugstr_w(path), debugstr_w(mode));

  /* map mode string to open() flags. "man fopen" for possibilities. */
  if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1)
      return NULL;

  LOCK_FILES();
  fd = MSVCRT__wsopen(path, open_flags, share, MSVCRT__S_IREAD | MSVCRT__S_IWRITE);
  if (fd < 0)
    file = NULL;
  else if ((file = msvcrt_alloc_fp()) && msvcrt_init_fp(file, fd, stream_flags)
   != -1)
    TRACE(":fd (%d) mode (%s) FILE* (%p)\n", fd, debugstr_w(mode), file);
  else if (file)
  {
    file->_flag = 0;
    file = NULL;
  }

  TRACE(":got (%p)\n",file);
  if (fd >= 0 && !file)
    MSVCRT__close(fd);
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *		_fsopen (MSVCRT.@)
 */
MSVCRT_FILE * CDECL MSVCRT__fsopen(const char *path, const char *mode, int share)
{
    MSVCRT_FILE *ret;
    MSVCRT_wchar_t *pathW = NULL, *modeW = NULL;

    if (path && !(pathW = msvcrt_wstrdupa(path))) {
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }
    if (mode && !(modeW = msvcrt_wstrdupa(mode)))
    {
        MSVCRT_free(pathW);
        MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }

    ret = MSVCRT__wfsopen(pathW, modeW, share);

    MSVCRT_free(pathW);
    MSVCRT_free(modeW);
    return ret;
}

/*********************************************************************
 *		fopen (MSVCRT.@)
 */
MSVCRT_FILE * CDECL MSVCRT_fopen(const char *path, const char *mode)
{
    return MSVCRT__fsopen( path, mode, MSVCRT__SH_DENYNO );
}

/*********************************************************************
 *              fopen_s (MSVCRT.@)
 */
int CDECL MSVCRT_fopen_s(MSVCRT_FILE** pFile,
        const char *filename, const char *mode)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(filename != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return MSVCRT_EINVAL;

    *pFile = MSVCRT_fopen(filename, mode);

    if(!*pFile)
        return *MSVCRT__errno();
    return 0;
}

/*********************************************************************
 *		_wfopen (MSVCRT.@)
 */
MSVCRT_FILE * CDECL MSVCRT__wfopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode)
{
    return MSVCRT__wfsopen( path, mode, MSVCRT__SH_DENYNO );
}

/*********************************************************************
 *		_wfopen_s (MSVCRT.@)
 */
int CDECL MSVCRT__wfopen_s(MSVCRT_FILE** pFile, const MSVCRT_wchar_t *filename,
        const MSVCRT_wchar_t *mode)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(filename != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return MSVCRT_EINVAL;

    *pFile = MSVCRT__wfopen(filename, mode);

    if(!*pFile)
        return *MSVCRT__errno();
    return 0;
}

/*********************************************************************
 *		_flsbuf (MSVCRT.@)
 */
int CDECL MSVCRT__flsbuf(int c, MSVCRT_FILE* file)
{
    /* Flush output buffer */
    if(file->_bufsiz == 0 && !(file->_flag & MSVCRT__IONBF)) {
        msvcrt_alloc_buffer(file);
    }
    if(!(file->_flag & MSVCRT__IOWRT)) {
        if(file->_flag & MSVCRT__IORW)
            file->_flag |= MSVCRT__IOWRT;
        else
            return MSVCRT_EOF;
    }
    if(file->_bufsiz) {
        int res = 0;

        if(file->_cnt <= 0)
            res = msvcrt_flush_buffer(file);
        if(!res) {
            *file->_ptr++ = c;
            file->_cnt--;
            res = msvcrt_flush_buffer(file);
        }

        return res ? res : c&0xff;
    } else {
        unsigned char cc=c;
        int len;
        /* set _cnt to 0 for unbuffered FILEs */
        file->_cnt = 0;
        len = MSVCRT__write(file->_file, &cc, 1);
        if (len == 1)
            return c & 0xff;
        file->_flag |= MSVCRT__IOERR;
        return MSVCRT_EOF;
    }
}

/*********************************************************************
 *		fputc (MSVCRT.@)
 */
int CDECL MSVCRT_fputc(int c, MSVCRT_FILE* file)
{
  int res;

  MSVCRT__lock_file(file);
  if(file->_cnt>0) {
    *file->_ptr++=c;
    file->_cnt--;
    if (c == '\n')
    {
      res = msvcrt_flush_buffer(file);
      MSVCRT__unlock_file(file);
      return res ? res : c;
    }
    else {
      MSVCRT__unlock_file(file);
      return c & 0xff;
    }
  } else {
    res = MSVCRT__flsbuf(c, file);
    MSVCRT__unlock_file(file);
    return res;
  }
}

/*********************************************************************
 *		_fputchar (MSVCRT.@)
 */
int CDECL MSVCRT__fputchar(int c)
{
  return MSVCRT_fputc(c, MSVCRT_stdout);
}

/*********************************************************************
 *		fread (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_fread(void *ptr, MSVCRT_size_t size, MSVCRT_size_t nmemb, MSVCRT_FILE* file)
{
  MSVCRT_size_t rcnt=size * nmemb;
  MSVCRT_size_t read=0;
  int pread=0;

  if(!rcnt)
	return 0;

  MSVCRT__lock_file(file);

  /* first buffered data */
  if(file->_cnt>0) {
	int pcnt= (rcnt>file->_cnt)? file->_cnt:rcnt;
	memcpy(ptr, file->_ptr, pcnt);
	file->_cnt -= pcnt;
	file->_ptr += pcnt;
	read += pcnt ;
	rcnt -= pcnt ;
        ptr = (char*)ptr + pcnt;
  } else if(!(file->_flag & MSVCRT__IOREAD )) {
	if(file->_flag & MSVCRT__IORW) {
		file->_flag |= MSVCRT__IOREAD;
	} else {
        MSVCRT__unlock_file(file);
        return 0;
    }
  }
  while(rcnt>0)
  {
    int i;
    /* Fill the buffer on small reads.
     * TODO: Use a better buffering strategy.
     */
    if (!file->_cnt && size*nmemb <= MSVCRT_BUFSIZ/2 && !(file->_flag & MSVCRT__IONBF)) {
      if (file->_bufsiz == 0) {
        msvcrt_alloc_buffer(file);
      }
      file->_cnt = MSVCRT__read(file->_file, file->_base, file->_bufsiz);
      file->_ptr = file->_base;
      i = (file->_cnt<rcnt) ? file->_cnt : rcnt;
      /* If the buffer fill reaches eof but fread wouldn't, clear eof. */
      if (i > 0 && i < file->_cnt) {
        msvcrt_get_ioinfo(file->_file)->wxflag &= ~WX_ATEOF;
        file->_flag &= ~MSVCRT__IOEOF;
      }
      if (i > 0) {
        memcpy(ptr, file->_ptr, i);
        file->_cnt -= i;
        file->_ptr += i;
      }
    } else {
      i = MSVCRT__read(file->_file,ptr, rcnt);
    }
    pread += i;
    rcnt -= i;
    ptr = (char *)ptr+i;
    /* expose feof condition in the flags
     * MFC tests file->_flag for feof, and doesn't call feof())
     */
    if (msvcrt_get_ioinfo(file->_file)->wxflag & WX_ATEOF)
        file->_flag |= MSVCRT__IOEOF;
    else if (i == -1)
    {
        file->_flag |= MSVCRT__IOERR;
        pread = 0;
        rcnt = 0;
    }
    if (i < 1) break;
  }
  read+=pread;
  MSVCRT__unlock_file(file);
  return read / size;
}

/*********************************************************************
 *		_wfreopen (MSVCRT.@)
 *
 */
MSVCRT_FILE* CDECL MSVCRT__wfreopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode, MSVCRT_FILE* file)
{
  int open_flags, stream_flags, fd;

  TRACE(":path (%s) mode (%s) file (%p) fd (%d)\n", debugstr_w(path), debugstr_w(mode), file, file->_file);

  LOCK_FILES();
  if (!file || ((fd = file->_file) < 0) || fd > MSVCRT_fdend)
    file = NULL;
  else
  {
    MSVCRT_fclose(file);
    /* map mode string to open() flags. "man fopen" for possibilities. */
    if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1)
      file = NULL;
    else
    {
      fd = MSVCRT__wopen(path, open_flags, MSVCRT__S_IREAD | MSVCRT__S_IWRITE);
      if (fd < 0)
        file = NULL;
      else if (msvcrt_init_fp(file, fd, stream_flags) == -1)
      {
          file->_flag = 0;
          WARN(":failed-last error (%d)\n",GetLastError());
          msvcrt_set_errno(GetLastError());
          file = NULL;
      }
    }
  }
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *      _wfreopen_s (MSVCRT.@)
 */
int CDECL MSVCRT__wfreopen_s(MSVCRT_FILE** pFile,
        const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode, MSVCRT_FILE* file)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(path != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_EINVAL;

    *pFile = MSVCRT__wfreopen(path, mode, file);

    if(!*pFile)
        return *MSVCRT__errno();
    return 0;
}

/*********************************************************************
 *      freopen (MSVCRT.@)
 *
 */
MSVCRT_FILE* CDECL MSVCRT_freopen(const char *path, const char *mode, MSVCRT_FILE* file)
{
    MSVCRT_FILE *ret;
    MSVCRT_wchar_t *pathW = NULL, *modeW = NULL;

    if (path && !(pathW = msvcrt_wstrdupa(path))) return NULL;
    if (mode && !(modeW = msvcrt_wstrdupa(mode)))
    {
        MSVCRT_free(pathW);
        return NULL;
    }

    ret = MSVCRT__wfreopen(pathW, modeW, file);

    MSVCRT_free(pathW);
    MSVCRT_free(modeW);
    return ret;
}

/*********************************************************************
 *      freopen_s (MSVCRT.@)
 */
int CDECL MSVCRT_freopen_s(MSVCRT_FILE** pFile,
        const char *path, const char *mode, MSVCRT_FILE* file)
{
    if (!MSVCRT_CHECK_PMT(pFile != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(path != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(mode != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_EINVAL;

    *pFile = MSVCRT_freopen(path, mode, file);

    if(!*pFile)
        return *MSVCRT__errno();
    return 0;
}

/*********************************************************************
 *		fsetpos (MSVCRT.@)
 */
int CDECL MSVCRT_fsetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
  int ret;

  MSVCRT__lock_file(file);
  /* Note that all this has been lifted 'as is' from fseek */
  if(file->_flag & MSVCRT__IOWRT)
	msvcrt_flush_buffer(file);

  /* Discard buffered input */
  file->_cnt = 0;
  file->_ptr = file->_base;
  
  /* Reset direction of i/o */
  if(file->_flag & MSVCRT__IORW) {
        file->_flag &= ~(MSVCRT__IOREAD|MSVCRT__IOWRT);
  }

  ret = (MSVCRT__lseeki64(file->_file,*pos,SEEK_SET) == -1) ? -1 : 0;
  MSVCRT__unlock_file(file);
  return ret;
}

/*********************************************************************
 *		_ftelli64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__ftelli64(MSVCRT_FILE* file)
{
    /* TODO: just call fgetpos and return lower half of result */
    int off=0;
    __int64 pos;

    MSVCRT__lock_file(file);
    pos = _telli64(file->_file);
    if(pos == -1) {
        MSVCRT__unlock_file(file);
        return -1;
    }
    if(file->_bufsiz)  {
        if( file->_flag & MSVCRT__IOWRT ) {
            off = file->_ptr - file->_base;
        } else {
            off = -file->_cnt;
            if (msvcrt_get_ioinfo(file->_file)->wxflag & WX_TEXT) {
                /* Black magic correction for CR removal */
                int i;
                for (i=0; i<file->_cnt; i++) {
                    if (file->_ptr[i] == '\n')
                        off--;
                }
                /* Black magic when reading CR at buffer boundary*/
                if(msvcrt_get_ioinfo(file->_file)->wxflag & WX_READCR)
                    off--;
            }
        }
    }

    MSVCRT__unlock_file(file);
    return off + pos;
}

/*********************************************************************
 *		ftell (MSVCRT.@)
 */
LONG CDECL MSVCRT_ftell(MSVCRT_FILE* file)
{
  return MSVCRT__ftelli64(file);
}

/*********************************************************************
 *		fgetpos (MSVCRT.@)
 */
int CDECL MSVCRT_fgetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
    int off=0;

    MSVCRT__lock_file(file);
    *pos = MSVCRT__lseeki64(file->_file,0,SEEK_CUR);
    if(*pos == -1) {
        MSVCRT__unlock_file(file);
        return -1;
    }
    if(file->_bufsiz)  {
        if( file->_flag & MSVCRT__IOWRT ) {
            off = file->_ptr - file->_base;
        } else {
            off = -file->_cnt;
            if (msvcrt_get_ioinfo(file->_file)->wxflag & WX_TEXT) {
                /* Black magic correction for CR removal */
                int i;
                for (i=0; i<file->_cnt; i++) {
                    if (file->_ptr[i] == '\n')
                        off--;
                }
                /* Black magic when reading CR at buffer boundary*/
                if(msvcrt_get_ioinfo(file->_file)->wxflag & WX_READCR)
                    off--;
            }
        }
    }
    *pos += off;
    MSVCRT__unlock_file(file);
    return 0;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int CDECL MSVCRT_fputs(const char *s, MSVCRT_FILE* file)
{
    MSVCRT_size_t i, len = strlen(s);
    int ret;

    MSVCRT__lock_file(file);
    if (!(msvcrt_get_ioinfo(file->_file)->wxflag & WX_TEXT)) {
      ret = MSVCRT_fwrite(s,sizeof(*s),len,file) == len ? 0 : MSVCRT_EOF;
      MSVCRT__unlock_file(file);
      return ret;
    }
    for (i=0; i<len; i++)
      if (MSVCRT_fputc(s[i], file) == MSVCRT_EOF)  {
        MSVCRT__unlock_file(file);
        return MSVCRT_EOF;
      }

    MSVCRT__unlock_file(file);
    return 0;
}

/*********************************************************************
 *		fputws (MSVCRT.@)
 */
int CDECL MSVCRT_fputws(const MSVCRT_wchar_t *s, MSVCRT_FILE* file)
{
    MSVCRT_size_t i, len = strlenW(s);
    int ret;

    MSVCRT__lock_file(file);
    if (!(msvcrt_get_ioinfo(file->_file)->wxflag & WX_TEXT)) {
        ret = MSVCRT_fwrite(s,sizeof(*s),len,file) == len ? 0 : MSVCRT_EOF;
        MSVCRT__unlock_file(file);
        return ret;
    }
    for (i=0; i<len; i++) {
        if (((s[i] == '\n') && (MSVCRT_fputc('\r', file) == MSVCRT_EOF))
                || MSVCRT_fputwc(s[i], file) == MSVCRT_WEOF) {
            MSVCRT__unlock_file(file);
            return MSVCRT_WEOF;
        }
    }

    MSVCRT__unlock_file(file);
    return 0;
}

/*********************************************************************
 *		getchar (MSVCRT.@)
 */
int CDECL MSVCRT_getchar(void)
{
  return MSVCRT_fgetc(MSVCRT_stdin);
}

/*********************************************************************
 *		getc (MSVCRT.@)
 */
int CDECL MSVCRT_getc(MSVCRT_FILE* file)
{
  return MSVCRT_fgetc(file);
}

/*********************************************************************
 *		gets (MSVCRT.@)
 */
char * CDECL MSVCRT_gets(char *buf)
{
  int    cc;
  char * buf_start = buf;

  MSVCRT__lock_file(MSVCRT_stdin);
  for(cc = MSVCRT_fgetc(MSVCRT_stdin); cc != MSVCRT_EOF && cc != '\n';
      cc = MSVCRT_fgetc(MSVCRT_stdin))
  if(cc != '\r') *buf++ = (char)cc;

  *buf = '\0';

  TRACE("got '%s'\n", buf_start);
  MSVCRT__unlock_file(MSVCRT_stdin);
  return buf_start;
}

/*********************************************************************
 *		_getws (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__getws(MSVCRT_wchar_t* buf)
{
    MSVCRT_wint_t cc;
    MSVCRT_wchar_t* ws = buf;

    MSVCRT__lock_file(MSVCRT_stdin);
    for (cc = MSVCRT_fgetwc(MSVCRT_stdin); cc != MSVCRT_WEOF && cc != '\n';
         cc = MSVCRT_fgetwc(MSVCRT_stdin))
    {
        if (cc != '\r')
            *buf++ = (MSVCRT_wchar_t)cc;
    }
    *buf = '\0';

    TRACE("got %s\n", debugstr_w(ws));
    MSVCRT__unlock_file(MSVCRT_stdin);
    return ws;
}

/*********************************************************************
 *		putc (MSVCRT.@)
 */
int CDECL MSVCRT_putc(int c, MSVCRT_FILE* file)
{
  return MSVCRT_fputc(c, file);
}

/*********************************************************************
 *		putchar (MSVCRT.@)
 */
int CDECL MSVCRT_putchar(int c)
{
  return MSVCRT_fputc(c, MSVCRT_stdout);
}

/*********************************************************************
 *		_putwch (MSVCRT.@)
 */
int CDECL MSVCRT__putwch(int c)
{
  return MSVCRT_fputwc(c, MSVCRT_stdout);
}

/*********************************************************************
 *		puts (MSVCRT.@)
 */
int CDECL MSVCRT_puts(const char *s)
{
    MSVCRT_size_t len = strlen(s);
    int ret;

    MSVCRT__lock_file(MSVCRT_stdout);
    if(MSVCRT_fwrite(s, sizeof(*s), len, MSVCRT_stdout) != len) {
        MSVCRT__unlock_file(MSVCRT_stdout);
        return MSVCRT_EOF;
    }

    ret = MSVCRT_fwrite("\n",1,1,MSVCRT_stdout) == 1 ? 0 : MSVCRT_EOF;
    MSVCRT__unlock_file(MSVCRT_stdout);
    return ret;
}

/*********************************************************************
 *		_putws (MSVCRT.@)
 */
int CDECL MSVCRT__putws(const MSVCRT_wchar_t *s)
{
    static const MSVCRT_wchar_t nl = '\n';
    MSVCRT_size_t len = strlenW(s);
    int ret;

    MSVCRT__lock_file(MSVCRT_stdout);
    if(MSVCRT_fwrite(s, sizeof(*s), len, MSVCRT_stdout) != len) {
        MSVCRT__unlock_file(MSVCRT_stdout);
        return MSVCRT_EOF;
    }

    ret = MSVCRT_fwrite(&nl,sizeof(nl),1,MSVCRT_stdout) == 1 ? 0 : MSVCRT_EOF;
    MSVCRT__unlock_file(MSVCRT_stdout);
    return ret;
}

/*********************************************************************
 *		remove (MSVCRT.@)
 */
int CDECL MSVCRT_remove(const char *path)
{
  TRACE("(%s)\n",path);
  if (DeleteFileA(path))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wremove (MSVCRT.@)
 */
int CDECL MSVCRT__wremove(const MSVCRT_wchar_t *path)
{
  TRACE("(%s)\n",debugstr_w(path));
  if (DeleteFileW(path))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		rename (MSVCRT.@)
 */
int CDECL MSVCRT_rename(const char *oldpath,const char *newpath)
{
  TRACE(":from %s to %s\n",oldpath,newpath);
  if (MoveFileExA(oldpath, newpath, MOVEFILE_COPY_ALLOWED))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wrename (MSVCRT.@)
 */
int CDECL MSVCRT__wrename(const MSVCRT_wchar_t *oldpath,const MSVCRT_wchar_t *newpath)
{
  TRACE(":from %s to %s\n",debugstr_w(oldpath),debugstr_w(newpath));
  if (MoveFileExW(oldpath, newpath, MOVEFILE_COPY_ALLOWED))
    return 0;
  TRACE(":failed (%d)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		setvbuf (MSVCRT.@)
 */
int CDECL MSVCRT_setvbuf(MSVCRT_FILE* file, char *buf, int mode, MSVCRT_size_t size)
{
  MSVCRT__lock_file(file);
  if(file->_bufsiz) {
	MSVCRT_free(file->_base);
	file->_bufsiz = 0;
	file->_cnt = 0;
  }
  if(mode == MSVCRT__IOFBF) {
	file->_flag &= ~MSVCRT__IONBF;
  	file->_base = file->_ptr = buf;
  	if(buf) {
		file->_bufsiz = size;
	}
  } else {
	file->_flag |= MSVCRT__IONBF;
  }
  MSVCRT__unlock_file(file);
  return 0;
}

/*********************************************************************
 *		setbuf (MSVCRT.@)
 */
void CDECL MSVCRT_setbuf(MSVCRT_FILE* file, char *buf)
{
  MSVCRT_setvbuf(file, buf, buf ? MSVCRT__IOFBF : MSVCRT__IONBF, MSVCRT_BUFSIZ);
}

/*********************************************************************
 *		tmpnam (MSVCRT.@)
 */
char * CDECL MSVCRT_tmpnam(char *s)
{
  char tmpstr[16];
  char *p;
  int count, size;

  if (!s) {
    thread_data_t *data = msvcrt_get_thread_data();

    if(!data->tmpnam_buffer)
      data->tmpnam_buffer = MSVCRT_malloc(MAX_PATH);

    s = data->tmpnam_buffer;
  }

  msvcrt_int_to_base32(GetCurrentProcessId(), tmpstr);
  p = s + sprintf(s, "\\s%s.", tmpstr);
  for (count = 0; count < MSVCRT_TMP_MAX; count++)
  {
    size = msvcrt_int_to_base32(tmpnam_unique++, tmpstr);
    memcpy(p, tmpstr, size);
    p[size] = '\0';
    if (GetFileAttributesA(s) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      break;
  }
  return s;
}

/*********************************************************************
 *              _wtmpnam (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT_wtmpnam(MSVCRT_wchar_t *s)
{
    static const MSVCRT_wchar_t format[] = {'\\','s','%','s','.',0};
    MSVCRT_wchar_t tmpstr[16];
    MSVCRT_wchar_t *p;
    int count, size;
    if (!s) {
        thread_data_t *data = msvcrt_get_thread_data();

        if(!data->wtmpnam_buffer)
            data->wtmpnam_buffer = MSVCRT_malloc(sizeof(MSVCRT_wchar_t[MAX_PATH]));

        s = data->wtmpnam_buffer;
    }

    msvcrt_int_to_base32_w(GetCurrentProcessId(), tmpstr);
    p = s + MSVCRT__snwprintf(s, MAX_PATH, format, tmpstr);
    for (count = 0; count < MSVCRT_TMP_MAX; count++)
    {
        size = msvcrt_int_to_base32_w(tmpnam_unique++, tmpstr);
        memcpy(p, tmpstr, size*sizeof(MSVCRT_wchar_t));
        p[size] = '\0';
        if (GetFileAttributesW(s) == INVALID_FILE_ATTRIBUTES &&
                GetLastError() == ERROR_FILE_NOT_FOUND)
            break;
    }
    return s;
}

/*********************************************************************
 *		tmpfile (MSVCRT.@)
 */
MSVCRT_FILE* CDECL MSVCRT_tmpfile(void)
{
  char *filename = MSVCRT_tmpnam(NULL);
  int fd;
  MSVCRT_FILE* file = NULL;

  LOCK_FILES();
  fd = MSVCRT__open(filename, MSVCRT__O_CREAT | MSVCRT__O_BINARY | MSVCRT__O_RDWR | MSVCRT__O_TEMPORARY);
  if (fd != -1 && (file = msvcrt_alloc_fp()))
  {
    if (msvcrt_init_fp(file, fd, MSVCRT__O_RDWR) == -1)
    {
        file->_flag = 0;
        file = NULL;
    }
    else file->_tmpfname = MSVCRT__strdup(filename);
  }
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *      tmpfile_s (MSVCRT.@)
 */
int CDECL MSVCRT_tmpfile_s(MSVCRT_FILE** file)
{
    if (!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_EINVAL;

    *file = MSVCRT_tmpfile();
    return 0;
}

static int puts_clbk_file_a(void *file, int len, const char *str)
{
    return MSVCRT_fwrite(str, sizeof(char), len, file);
}

static int puts_clbk_file_w(void *file, int len, const MSVCRT_wchar_t *str)
{
    return MSVCRT_fwrite(str, sizeof(MSVCRT_wchar_t), len, file);
}

/*********************************************************************
 *		vfprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vfprintf(MSVCRT_FILE* file, const char *format, __ms_va_list valist)
{
    int ret;

    MSVCRT__lock_file(file);
    ret = pf_printf_a(puts_clbk_file_a, file, format, NULL, FALSE, FALSE, arg_clbk_valist, NULL, &valist);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		vfprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vfprintf_s(MSVCRT_FILE* file, const char *format, __ms_va_list valist)
{
    int ret;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return -1;

    MSVCRT__lock_file(file);
    ret = pf_printf_a(puts_clbk_file_a, file, format, NULL, FALSE, TRUE, arg_clbk_valist, NULL, &valist);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		vfwprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vfwprintf(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    int ret;

    MSVCRT__lock_file(file);
    ret = pf_printf_w(puts_clbk_file_w, file, format, NULL, FALSE, FALSE, arg_clbk_valist, NULL, &valist);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		vfwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vfwprintf_s(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    int ret;

    if (!MSVCRT_CHECK_PMT( file != NULL )) return -1;

    MSVCRT__lock_file(file);
    ret = pf_printf_w(puts_clbk_file_w, file, format, NULL, FALSE, TRUE, arg_clbk_valist, NULL, &valist);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		vprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vprintf(const char *format, __ms_va_list valist)
{
  return MSVCRT_vfprintf(MSVCRT_stdout,format,valist);
}

/*********************************************************************
 *		vprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vprintf_s(const char *format, __ms_va_list valist)
{
  return MSVCRT_vfprintf_s(MSVCRT_stdout,format,valist);
}

/*********************************************************************
 *		vwprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vwprintf(const MSVCRT_wchar_t *format, __ms_va_list valist)
{
  return MSVCRT_vfwprintf(MSVCRT_stdout,format,valist);
}

/*********************************************************************
 *		vwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vwprintf_s(const MSVCRT_wchar_t *format, __ms_va_list valist)
{
  return MSVCRT_vfwprintf_s(MSVCRT_stdout,format,valist);
}

/*********************************************************************
 *		fprintf (MSVCRT.@)
 */
int CDECL MSVCRT_fprintf(MSVCRT_FILE* file, const char *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vfprintf(file, format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		fprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_fprintf_s(MSVCRT_FILE* file, const char *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vfprintf_s(file, format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		fwprintf (MSVCRT.@)
 */
int CDECL MSVCRT_fwprintf(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vfwprintf(file, format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		fwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_fwprintf_s(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vfwprintf_s(file, format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		printf (MSVCRT.@)
 */
int CDECL MSVCRT_printf(const char *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vfprintf(MSVCRT_stdout, format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		printf_s (MSVCRT.@)
 */
int CDECL MSVCRT_printf_s(const char *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vprintf_s(format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		ungetc (MSVCRT.@)
 */
int CDECL MSVCRT_ungetc(int c, MSVCRT_FILE * file)
{
    if (c == MSVCRT_EOF)
        return MSVCRT_EOF;

    MSVCRT__lock_file(file);
    if(file->_bufsiz == 0) {
        msvcrt_alloc_buffer(file);
        file->_ptr++;
    }
    if(file->_ptr>file->_base) {
        file->_ptr--;
        *file->_ptr=c;
        file->_cnt++;
        MSVCRT_clearerr(file);
        MSVCRT__unlock_file(file);
        return c;
    }

    MSVCRT__unlock_file(file);
    return MSVCRT_EOF;
}

/*********************************************************************
 *              ungetwc (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT_ungetwc(MSVCRT_wint_t wc, MSVCRT_FILE * file)
{
    MSVCRT_wchar_t mwc = wc;
    char * pp = (char *)&mwc;
    int i;

    MSVCRT__lock_file(file);
    for(i=sizeof(MSVCRT_wchar_t)-1;i>=0;i--) {
        if(pp[i] != MSVCRT_ungetc(pp[i],file)) {
            MSVCRT__unlock_file(file);
            return MSVCRT_WEOF;
        }
    }

    MSVCRT__unlock_file(file);
    return mwc;
}

/*********************************************************************
 *		wprintf (MSVCRT.@)
 */
int CDECL MSVCRT_wprintf(const MSVCRT_wchar_t *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vwprintf(format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		wprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_wprintf_s(const MSVCRT_wchar_t *format, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, format);
    res = MSVCRT_vwprintf_s(format, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		_getmaxstdio (MSVCRT.@)
 */
int CDECL MSVCRT__getmaxstdio(void)
{
    return MSVCRT_max_streams;
}

/*********************************************************************
 *		_setmaxstdio (MSVCRT.@)
 */
int CDECL MSVCRT__setmaxstdio(int newmax)
{
    TRACE("%d\n", newmax);

    if(newmax<_IOB_ENTRIES || newmax>MSVCRT_MAX_FILES || newmax<MSVCRT_stream_idx)
        return -1;

    MSVCRT_max_streams = newmax;
    return MSVCRT_max_streams;
}
