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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "winreg.h"
#include "winternl.h"
#include "msvcrt.h"

#include "wine/unicode.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* for stat mode, permissions apply to all,owner and group */
#define ALL_S_IREAD  (MSVCRT__S_IREAD  | (MSVCRT__S_IREAD  >> 3) | (MSVCRT__S_IREAD  >> 6))
#define ALL_S_IWRITE (MSVCRT__S_IWRITE | (MSVCRT__S_IWRITE >> 3) | (MSVCRT__S_IWRITE >> 6))
#define ALL_S_IEXEC  (MSVCRT__S_IEXEC  | (MSVCRT__S_IEXEC  >> 3) | (MSVCRT__S_IEXEC  >> 6))

/* _access() bit flags FIXME: incomplete */
#define MSVCRT_W_OK      0x02

/* values for wxflag in file descriptor */
#define WX_OPEN           0x01
#define WX_ATEOF          0x02
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TEXT           0x80

/* FIXME: this should be allocated dynamically */
#define MSVCRT_MAX_FILES 2048

typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    DWORD               unkn[7]; /* critical section and init flag */       
} ioinfo;

static ioinfo MSVCRT_fdesc[MSVCRT_MAX_FILES];

MSVCRT_FILE MSVCRT__iob[3];

static int MSVCRT_fdstart = 3; /* first unallocated fd */
static int MSVCRT_fdend = 3; /* highest allocated fd */

static MSVCRT_FILE* MSVCRT_fstreams[2048];
static int   MSVCRT_stream_idx;

/* INTERNAL: process umask */
static int MSVCRT_umask = 0;

/* INTERNAL: Static buffer for temp file name */
static char MSVCRT_tmpname[MAX_PATH];

static const unsigned int EXE = 'e' << 16 | 'x' << 8 | 'e';
static const unsigned int BAT = 'b' << 16 | 'a' << 8 | 't';
static const unsigned int CMD = 'c' << 16 | 'm' << 8 | 'd';
static const unsigned int COM = 'c' << 16 | 'o' << 8 | 'm';

#define TOUL(x) (ULONGLONG)(x)
static const ULONGLONG WCEXE = TOUL('e') << 32 | TOUL('x') << 16 | TOUL('e');
static const ULONGLONG WCBAT = TOUL('b') << 32 | TOUL('a') << 16 | TOUL('t');
static const ULONGLONG WCCMD = TOUL('c') << 32 | TOUL('m') << 16 | TOUL('d');
static const ULONGLONG WCCOM = TOUL('c') << 32 | TOUL('o') << 16 | TOUL('m');

/* This critical section protects the tables MSVCRT_fdesc and MSVCRT_fstreams,
 * and their related indexes, MSVCRT_fdstart, MSVCRT_fdend,
 * and MSVCRT_stream_idx, from race conditions.
 * It doesn't protect against race conditions manipulating the underlying files
 * or flags; doing so would probably be better accomplished with per-file
 * protection, rather than locking the whole table for every change.
 */
static CRITICAL_SECTION MSVCRT_file_cs;
#define LOCK_FILES()    do { EnterCriticalSection(&MSVCRT_file_cs); } while (0)
#define UNLOCK_FILES()  do { LeaveCriticalSection(&MSVCRT_file_cs); } while (0)

static void msvcrt_cp_from_stati64(const struct MSVCRT__stati64 *bufi64, struct MSVCRT__stat *buf)
{
    buf->st_dev   = bufi64->st_dev;
    buf->st_ino   = bufi64->st_ino;
    buf->st_mode  = bufi64->st_mode;
    buf->st_nlink = bufi64->st_nlink;
    buf->st_uid   = bufi64->st_uid;
    buf->st_gid   = bufi64->st_gid;
    buf->st_rdev  = bufi64->st_rdev;
    buf->st_size  = bufi64->st_size;
    buf->st_atime = bufi64->st_atime;
    buf->st_mtime = bufi64->st_mtime;
    buf->st_ctime = bufi64->st_ctime;
}

static inline BOOL msvcrt_is_valid_fd(int fd)
{
  return fd >= 0 && fd < MSVCRT_fdend && (MSVCRT_fdesc[fd].wxflag & WX_OPEN);
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
  if (MSVCRT_fdesc[fd].handle == INVALID_HANDLE_VALUE) FIXME("wtf\n");
  return MSVCRT_fdesc[fd].handle;
}

/* INTERNAL: free a file entry fd */
static void msvcrt_free_fd(int fd)
{
  LOCK_FILES();
  MSVCRT_fdesc[fd].handle = INVALID_HANDLE_VALUE;
  MSVCRT_fdesc[fd].wxflag = 0;
  TRACE(":fd (%d) freed\n",fd);
  if (fd < 3) /* don't use 0,1,2 for user files */
  {
    switch (fd)
    {
    case 0: SetStdHandle(STD_INPUT_HANDLE,  NULL); break;
    case 1: SetStdHandle(STD_OUTPUT_HANDLE, NULL); break;
    case 2: SetStdHandle(STD_ERROR_HANDLE,  NULL); break;
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
  if (fd >= MSVCRT_MAX_FILES)
  {
    WARN(":files exhausted!\n");
    return -1;
  }
  MSVCRT_fdesc[fd].handle = hand;
  MSVCRT_fdesc[fd].wxflag = WX_OPEN | (flag & (WX_DONTINHERIT | WX_APPEND | WX_TEXT));

  /* locate next free slot */
  if (fd == MSVCRT_fdstart && fd == MSVCRT_fdend)
    MSVCRT_fdstart = MSVCRT_fdend + 1;
  else
    while (MSVCRT_fdstart < MSVCRT_fdend &&
     MSVCRT_fdesc[MSVCRT_fdstart].handle != INVALID_HANDLE_VALUE)
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

  for (i = 3; i < sizeof(MSVCRT_fstreams) / sizeof(MSVCRT_fstreams[0]); i++)
  {
    if (!MSVCRT_fstreams[i] || MSVCRT_fstreams[i]->_flag == 0)
    {
      if (!MSVCRT_fstreams[i])
      {
        if (!(MSVCRT_fstreams[i] = MSVCRT_calloc(sizeof(MSVCRT_FILE),1)))
          return NULL;
        if (i == MSVCRT_stream_idx) MSVCRT_stream_idx++;
      }
      return MSVCRT_fstreams[i];
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
unsigned msvcrt_create_io_inherit_block(STARTUPINFOA* si)
{
  int         fd;
  char*       wxflag_ptr;
  HANDLE*     handle_ptr;

  si->cbReserved2 = sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * MSVCRT_fdend;
  si->lpReserved2 = MSVCRT_calloc(si->cbReserved2, 1);
  if (!si->lpReserved2)
  {
    si->cbReserved2 = 0;
    return FALSE;
  }
  wxflag_ptr = (char*)si->lpReserved2 + sizeof(unsigned);
  handle_ptr = (HANDLE*)(wxflag_ptr + MSVCRT_fdend * sizeof(char));

  *(unsigned*)si->lpReserved2 = MSVCRT_fdend;
  for (fd = 0; fd < MSVCRT_fdend; fd++)
  {
    /* to be inherited, we need it to be open, and that DONTINHERIT isn't set */
    if ((MSVCRT_fdesc[fd].wxflag & (WX_OPEN | WX_DONTINHERIT)) == WX_OPEN)
    {
      *wxflag_ptr = MSVCRT_fdesc[fd].wxflag;
      *handle_ptr = MSVCRT_fdesc[fd].handle;
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

  InitializeCriticalSection(&MSVCRT_file_cs);
  GetStartupInfoA(&si);
  if (si.cbReserved2 != 0 && si.lpReserved2 != NULL)
  {
    char*       wxflag_ptr;
    HANDLE*     handle_ptr;

    MSVCRT_fdend = *(unsigned*)si.lpReserved2;

    wxflag_ptr = (char*)(si.lpReserved2 + sizeof(unsigned));
    handle_ptr = (HANDLE*)(wxflag_ptr + MSVCRT_fdend * sizeof(char));

    MSVCRT_fdend = min(MSVCRT_fdend, sizeof(MSVCRT_fdesc) / sizeof(MSVCRT_fdesc[0]));
    for (i = 0; i < MSVCRT_fdend; i++)
    {
      if ((*wxflag_ptr & WX_OPEN) && *handle_ptr != INVALID_HANDLE_VALUE)
      {
        MSVCRT_fdesc[i].wxflag  = *wxflag_ptr;
        MSVCRT_fdesc[i].handle = *handle_ptr;
      }
      else
      {
        MSVCRT_fdesc[i].wxflag  = 0;
        MSVCRT_fdesc[i].handle = INVALID_HANDLE_VALUE;
      }
      wxflag_ptr++; handle_ptr++;
    }
    for (MSVCRT_fdstart = 3; MSVCRT_fdstart < MSVCRT_fdend; MSVCRT_fdstart++)
        if (MSVCRT_fdesc[MSVCRT_fdstart].handle == INVALID_HANDLE_VALUE) break;
  }

  if (!(MSVCRT_fdesc[0].wxflag & WX_OPEN) || MSVCRT_fdesc[0].handle == INVALID_HANDLE_VALUE)
  {
    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
                    GetCurrentProcess(), &MSVCRT_fdesc[0].handle, 0, TRUE, 
                    DUPLICATE_SAME_ACCESS);
    MSVCRT_fdesc[0].wxflag = WX_OPEN | WX_TEXT;
  }
  if (!(MSVCRT_fdesc[1].wxflag & WX_OPEN) || MSVCRT_fdesc[1].handle == INVALID_HANDLE_VALUE)
  {
    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_OUTPUT_HANDLE),
                    GetCurrentProcess(), &MSVCRT_fdesc[1].handle, 0, TRUE, 
                    DUPLICATE_SAME_ACCESS);
    MSVCRT_fdesc[1].wxflag = WX_OPEN | WX_TEXT;
  }
  if (!(MSVCRT_fdesc[2].wxflag & WX_OPEN) || MSVCRT_fdesc[2].handle == INVALID_HANDLE_VALUE)
  {
    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_ERROR_HANDLE),
                    GetCurrentProcess(), &MSVCRT_fdesc[2].handle, 0, TRUE, 
                    DUPLICATE_SAME_ACCESS);
    MSVCRT_fdesc[2].wxflag = WX_OPEN | WX_TEXT;
  }

  TRACE(":handles (%p)(%p)(%p)\n",MSVCRT_fdesc[0].handle,
	MSVCRT_fdesc[1].handle,MSVCRT_fdesc[2].handle);

  memset(MSVCRT__iob,0,3*sizeof(MSVCRT_FILE));
  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    MSVCRT_fstreams[i] = &MSVCRT__iob[i];
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
        if(cnt>0 && _write(file->_file, file->_base, cnt) != cnt) {
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
static void msvcrt_int_to_base32(int num, char *str)
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
}

/*********************************************************************
 *		__p__iob(MSVCRT.@)
 */
MSVCRT_FILE *__p__iob(void)
{
 return &MSVCRT__iob[0];
}

/*********************************************************************
 *		_access (MSVCRT.@)
 */
int _access(const char *filename, int mode)
{
  DWORD attr = GetFileAttributesA(filename);

  TRACE("(%s,%d) %ld\n",filename,mode,attr);

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
 *		_waccess (MSVCRT.@)
 */
int _waccess(const MSVCRT_wchar_t *filename, int mode)
{
  DWORD attr = GetFileAttributesW(filename);

  TRACE("(%s,%d) %ld\n",debugstr_w(filename),mode,attr);

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
 *		_chmod (MSVCRT.@)
 */
int _chmod(const char *path, int flags)
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
int _wchmod(const MSVCRT_wchar_t *path, int flags)
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
int _unlink(const char *path)
{
  TRACE("%s\n",debugstr_a(path));
  if(DeleteFileA(path))
    return 0;
  TRACE("failed (%ld)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wunlink (MSVCRT.@)
 */
int _wunlink(const MSVCRT_wchar_t *path)
{
  TRACE("(%s)\n",debugstr_w(path));
  if(DeleteFileW(path))
    return 0;
  TRACE("failed (%ld)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/* _flushall calls MSVCRT_fflush which calls _flushall */
int MSVCRT_fflush(MSVCRT_FILE* file);

/*********************************************************************
 *		_flushall (MSVCRT.@)
 */
int _flushall(void)
{
  int i, num_flushed = 0;

  LOCK_FILES();
  for (i = 3; i < MSVCRT_stream_idx; i++)
    if (MSVCRT_fstreams[i] && MSVCRT_fstreams[i]->_flag)
    {
#if 0
      /* FIXME: flush, do not commit */
      if (_commit(i) == -1)
	if (MSVCRT_fstreams[i])
	  MSVCRT_fstreams[i]->_flag |= MSVCRT__IOERR;
#endif
      if(MSVCRT_fstreams[i]->_flag & MSVCRT__IOWRT) {
	MSVCRT_fflush(MSVCRT_fstreams[i]);
        num_flushed++;
      }
    }
  UNLOCK_FILES();

  TRACE(":flushed (%d) handles\n",num_flushed);
  return num_flushed;
}

/*********************************************************************
 *		fflush (MSVCRT.@)
 */
int MSVCRT_fflush(MSVCRT_FILE* file)
{
  if(!file) {
	_flushall();
  } else if(file->_flag & MSVCRT__IOWRT) {
  	int res=msvcrt_flush_buffer(file);
  	return res;
  }
  return 0;
}

/*********************************************************************
 *		_close (MSVCRT.@)
 */
int _close(int fd)
{
  HANDLE hand;
  int ret;

  LOCK_FILES();
  hand = msvcrt_fdtoh(fd);
  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    ret = -1;
  else if (!CloseHandle(hand))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    msvcrt_set_errno(GetLastError());
    ret = -1;
  }
  else
  {
    msvcrt_free_fd(fd);
    ret = 0;
  }
  UNLOCK_FILES();
  TRACE(":ok\n");
  return ret;
}

/*********************************************************************
 *		_commit (MSVCRT.@)
 */
int _commit(int fd)
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
    TRACE(":failed-last error (%ld)\n",GetLastError());
    msvcrt_set_errno(GetLastError());
    return -1;
  }
  TRACE(":ok\n");
  return 0;
}

/*********************************************************************
 *		_dup2 (MSVCRT.@)
 * NOTES
 * MSDN isn't clear on this point, but the remarks for _pipe,
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclib/html/_crt__pipe.asp
 * indicate file descriptors duplicated with _dup and _dup2 are always
 * inheritable.
 */
int _dup2(int od, int nd)
{
  int ret;

  TRACE("(od=%d, nd=%d)\n", od, nd);
  LOCK_FILES();
  if (nd < MSVCRT_MAX_FILES && msvcrt_is_valid_fd(od))
  {
    HANDLE handle;

    if (DuplicateHandle(GetCurrentProcess(), MSVCRT_fdesc[od].handle,
     GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
      int wxflag = MSVCRT_fdesc[od].wxflag & ~MSVCRT__O_NOINHERIT;

      if (msvcrt_is_valid_fd(nd))
        _close(nd);
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
int _dup(int od)
{
  int fd, ret;
 
  LOCK_FILES();
  fd = MSVCRT_fdstart;
  if (_dup2(od, fd) == 0)
    ret = fd;
  else
    ret = -1;
  UNLOCK_FILES();
  return ret;
}

/*********************************************************************
 *		_eof (MSVCRT.@)
 */
int _eof(int fd)
{
  DWORD curpos,endpos;
  LONG hcurpos,hendpos;
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (MSVCRT_fdesc[fd].wxflag & WX_ATEOF) return TRUE;

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
int MSVCRT__fcloseall(void)
{
  int num_closed = 0, i;

  LOCK_FILES();
  for (i = 3; i < MSVCRT_stream_idx; i++)
    if (MSVCRT_fstreams[i] && MSVCRT_fstreams[i]->_flag &&
        !MSVCRT_fclose(MSVCRT_fstreams[i]))
      num_closed++;
  UNLOCK_FILES();

  TRACE(":closed (%d) handles\n",num_closed);
  return num_closed;
}

/* free everything on process exit */
void msvcrt_free_io(void)
{
    MSVCRT__fcloseall();
    /* The Win32 _fcloseall() function explicitly doesn't close stdin,
     * stdout, and stderr (unlike GNU), so we need to fclose() them here
     * or they won't get flushed.
     */
    MSVCRT_fclose(&MSVCRT__iob[0]);
    MSVCRT_fclose(&MSVCRT__iob[1]);
    MSVCRT_fclose(&MSVCRT__iob[2]);
    DeleteCriticalSection(&MSVCRT_file_cs);
}

/*********************************************************************
 *		_lseeki64 (MSVCRT.@)
 */
__int64 _lseeki64(int fd, __int64 offset, int whence)
{
  HANDLE hand = msvcrt_fdtoh(fd);
  LARGE_INTEGER ofs, ret;

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

  ofs.QuadPart = offset;
  if (SetFilePointerEx(hand, ofs, &ret, whence))
  {
    MSVCRT_fdesc[fd].wxflag &= ~WX_ATEOF;
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    return ret.QuadPart;
  }
  TRACE(":error-last error (%ld)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_lseek (MSVCRT.@)
 */
LONG _lseek(int fd, LONG offset, int whence)
{
    return _lseeki64(fd, offset, whence);
}

/*********************************************************************
 *		_locking (MSVCRT.@)
 *
 * This is untested; the underlying LockFile doesn't work yet.
 */
int _locking(int fd, int mode, LONG nbytes)
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

  TRACE(":fd (%d) by 0x%08lx mode %s\n",
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
 *		fseek (MSVCRT.@)
 */
int MSVCRT_fseek(MSVCRT_FILE* file, long offset, int whence)
{
  /* Flush output if needed */
  if(file->_flag & MSVCRT__IOWRT)
	msvcrt_flush_buffer(file);

  if(whence == SEEK_CUR && file->_flag & MSVCRT__IOREAD ) {
	offset -= file->_cnt;
  }
  /* Discard buffered input */
  file->_cnt = 0;
  file->_ptr = file->_base;
  /* Reset direction of i/o */
  if(file->_flag & MSVCRT__IORW) {
        file->_flag &= ~(MSVCRT__IOREAD|MSVCRT__IOWRT);
  }
  return (_lseek(file->_file,offset,whence) == -1)?-1:0;
}

/*********************************************************************
 *		_chsize (MSVCRT.@)
 */
int _chsize(int fd, long size)
{
    LONG cur, pos;
    HANDLE handle;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%ld)\n", fd, size);

    LOCK_FILES();

    handle = msvcrt_fdtoh(fd);
    if (handle != INVALID_HANDLE_VALUE)
    {
        /* save the current file pointer */
        cur = _lseek(fd, 0, SEEK_CUR);
        if (cur >= 0)
        {
            pos = _lseek(fd, size, SEEK_SET);
            if (pos >= 0)
            {
                ret = SetEndOfFile(handle);
                if (!ret) msvcrt_set_errno(GetLastError());
            }

            /* restore the file pointer */
            _lseek(fd, cur, SEEK_SET);
        }
    }

    UNLOCK_FILES();
    return ret ? 0 : -1;
}

/*********************************************************************
 *		clearerr (MSVCRT.@)
 */
void MSVCRT_clearerr(MSVCRT_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);
  file->_flag &= ~(MSVCRT__IOERR | MSVCRT__IOEOF);
}

/*********************************************************************
 *		rewind (MSVCRT.@)
 */
void MSVCRT_rewind(MSVCRT_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);
  MSVCRT_fseek(file, 0L, SEEK_SET);
  MSVCRT_clearerr(file);
}

static int msvcrt_get_flags(const char* mode, int *open_flags, int* stream_flags)
{
  int plus = strchr(mode, '+') != NULL;

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
    return -1;
  }

  while (*mode)
    switch (*mode++)
    {
    case 'B': case 'b':
      *open_flags |=  MSVCRT__O_BINARY;
      *open_flags &= ~MSVCRT__O_TEXT;
      break;
    case 'T': case 't':
      *open_flags |=  MSVCRT__O_TEXT;
      *open_flags &= ~MSVCRT__O_BINARY;
      break;
    case '+':
      break;
    default:
      FIXME(":unknown flag %c not supported\n",mode[-1]);
    }
  return 0;
}

/*********************************************************************
 *		_fdopen (MSVCRT.@)
 */
MSVCRT_FILE* MSVCRT__fdopen(int fd, const char *mode)
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
  else TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,mode,file);
  UNLOCK_FILES();

  return file;
}

/*********************************************************************
 *		_wfdopen (MSVCRT.@)
 */
MSVCRT_FILE* MSVCRT__wfdopen(int fd, const MSVCRT_wchar_t *mode)
{
  unsigned mlen = strlenW(mode);
  char *modea = MSVCRT_calloc(mlen + 1, 1);
  MSVCRT_FILE* file = NULL;
  int open_flags, stream_flags;

  if (modea &&
      WideCharToMultiByte(CP_ACP,0,mode,mlen,modea,mlen,NULL,NULL))
  {
      if (msvcrt_get_flags(modea, &open_flags, &stream_flags) == -1) return NULL;
      LOCK_FILES();
      if (!(file = msvcrt_alloc_fp()))
        file = NULL;
      else if (msvcrt_init_fp(file, fd, stream_flags) == -1)
      {
        file->_flag = 0;
        file = NULL;
      }
      else
      {
        if (file)
          MSVCRT_rewind(file); /* FIXME: is this needed ??? */
        TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,debugstr_w(mode),file);
      }
      UNLOCK_FILES();
  }
  return file;
}

/*********************************************************************
 *		_filelength (MSVCRT.@)
 */
LONG _filelength(int fd)
{
  LONG curPos = _lseek(fd, 0, SEEK_CUR);
  if (curPos != -1)
  {
    LONG endPos = _lseek(fd, 0, SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        _lseek(fd, curPos, SEEK_SET);
      return endPos;
    }
  }
  return -1;
}

/*********************************************************************
 *		_filelengthi64 (MSVCRT.@)
 */
__int64 _filelengthi64(int fd)
{
  __int64 curPos = _lseeki64(fd, 0, SEEK_CUR);
  if (curPos != -1)
  {
    __int64 endPos = _lseeki64(fd, 0, SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        _lseeki64(fd, curPos, SEEK_SET);
      return endPos;
    }
  }
  return -1;
}

/*********************************************************************
 *		_fileno (MSVCRT.@)
 */
int MSVCRT__fileno(MSVCRT_FILE* file)
{
  TRACE(":FILE* (%p) fd (%d)\n",file,file->_file);
  return file->_file;
}

/*********************************************************************
 *		_fstati64 (MSVCRT.@)
 */
int MSVCRT__fstati64(int fd, struct MSVCRT__stati64* buf)
{
  DWORD dw;
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
  memset(buf, 0, sizeof(struct MSVCRT__stati64));
  if (!GetFileInformationByHandle(hand, &hfi))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    msvcrt_set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }
  dw = GetFileType(hand);
  buf->st_mode = S_IREAD;
  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    buf->st_mode |= S_IWRITE;
  /* interestingly, Windows never seems to set S_IFDIR */
  if (dw == FILE_TYPE_CHAR)
    buf->st_mode |= S_IFCHR;
  else if (dw == FILE_TYPE_PIPE)
    buf->st_mode |= S_IFIFO;
  else
    buf->st_mode |= S_IFREG;
  TRACE(":dwFileAttributes = 0x%lx, mode set to 0x%x\n",hfi.dwFileAttributes,
   buf->st_mode);
  buf->st_nlink = hfi.nNumberOfLinks;
  buf->st_size  = ((__int64)hfi.nFileSizeHigh << 32) + hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  return 0;
}

/*********************************************************************
 *		_fstat (MSVCRT.@)
 */
int MSVCRT__fstat(int fd, struct MSVCRT__stat* buf)
{ int ret;
  struct MSVCRT__stati64 bufi64;

  ret = MSVCRT__fstati64(fd, &bufi64);
  if (!ret)
      msvcrt_cp_from_stati64(&bufi64, buf);
  return ret;
}

/*********************************************************************
 *		_futime (MSVCRT.@)
 */
int _futime(int fd, struct MSVCRT__utimbuf *t)
{
  HANDLE hand = msvcrt_fdtoh(fd);
  FILETIME at, wt;

  if (!t)
  {
    MSVCRT_time_t currTime;
    MSVCRT_time(&currTime);
    RtlSecondsSince1970ToTime(currTime, (LARGE_INTEGER *)&at);
    memcpy(&wt, &at, sizeof(wt));
  }
  else
  {
    RtlSecondsSince1970ToTime(t->actime, (LARGE_INTEGER *)&at);
    if (t->actime == t->modtime)
      memcpy(&wt, &at, sizeof(wt));
    else
      RtlSecondsSince1970ToTime(t->modtime, (LARGE_INTEGER *)&wt);
  }

  if (!SetFileTime(hand, NULL, &at, &wt))
  {
    msvcrt_set_errno(GetLastError());
    return -1 ;
  }
  return 0;
}

/*********************************************************************
 *		_get_osfhandle (MSVCRT.@)
 */
long _get_osfhandle(int fd)
{
  HANDLE hand = msvcrt_fdtoh(fd);
  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  return (long)hand;
}

/*********************************************************************
 *		_isatty (MSVCRT.@)
 */
int _isatty(int fd)
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
char *_mktemp(char *pattern)
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
MSVCRT_wchar_t *_wmktemp(MSVCRT_wchar_t *pattern)
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
int _pipe(int *pfds, unsigned int psize, int textmode)
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
        _close(pfds[0]);
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
 *              _sopen (MSVCRT.@)
 */
int MSVCRT__sopen( const char *path, int oflags, int shflags, ... )
{
  va_list ap;
  int pmode;
  DWORD access = 0, creation = 0, attrib;
  DWORD sharing;
  int wxflag = 0, fd;
  HANDLE hand;
  SECURITY_ATTRIBUTES sa;


  TRACE(":file (%s) oflags: 0x%04x shflags: 0x%04x\n",
        path, oflags, shflags);

  wxflag = split_oflags(oflags);
  switch (oflags & (MSVCRT__O_RDONLY | MSVCRT__O_WRONLY | MSVCRT__O_RDWR))
  {
  case MSVCRT__O_RDONLY: access |= GENERIC_READ; break;
  case MSVCRT__O_WRONLY: access |= GENERIC_WRITE; break;
  case MSVCRT__O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
  }

  if (oflags & MSVCRT__O_CREAT)
  {
    va_start(ap, shflags);
      pmode = va_arg(ap, int);
    va_end(ap);

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
      return -1;
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
    WARN(":failed-last error (%ld)\n",GetLastError());
    msvcrt_set_errno(GetLastError());
    return -1;
  }

  fd = msvcrt_alloc_fd(hand, wxflag);

  TRACE(":fd (%d) handle (%p)\n",fd, hand);
  return fd;
}

/*********************************************************************
 *              _wsopen (MSVCRT.@)
 */
int MSVCRT__wsopen( const MSVCRT_wchar_t* path, int oflags, int shflags, ... )
{
  const unsigned int len = strlenW(path);
  char *patha = MSVCRT_calloc(len + 1,1);
  va_list ap;
  int pmode;

  va_start(ap, shflags);
  pmode = va_arg(ap, int);
  va_end(ap);

  if (patha && WideCharToMultiByte(CP_ACP,0,path,len,patha,len,NULL,NULL))
  {
    int retval = MSVCRT__sopen(patha,oflags,shflags,pmode);
    MSVCRT_free(patha);
    return retval;
  }

  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *              _open (MSVCRT.@)
 */
int _open( const char *path, int flags, ... )
{
  va_list ap;

  if (flags & MSVCRT__O_CREAT)
  {
    int pmode;
    va_start(ap, flags);
    pmode = va_arg(ap, int);
    va_end(ap);
    return MSVCRT__sopen( path, flags, MSVCRT__SH_DENYNO, pmode );
  }
  else
    return MSVCRT__sopen( path, flags, MSVCRT__SH_DENYNO);
}

/*********************************************************************
 *              _wopen (MSVCRT.@)
 */
int _wopen(const MSVCRT_wchar_t *path,int flags,...)
{
  const unsigned int len = strlenW(path);
  char *patha = MSVCRT_calloc(len + 1,1);
  va_list ap;
  int pmode;

  va_start(ap, flags);
  pmode = va_arg(ap, int);
  va_end(ap);

  if (patha && WideCharToMultiByte(CP_ACP,0,path,len,patha,len,NULL,NULL))
  {
    int retval = _open(patha,flags,pmode);
    MSVCRT_free(patha);
    return retval;
  }

  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_creat (MSVCRT.@)
 */
int _creat(const char *path, int flags)
{
  int usedFlags = (flags & MSVCRT__O_TEXT)| MSVCRT__O_CREAT| MSVCRT__O_WRONLY| MSVCRT__O_TRUNC;
  return _open(path, usedFlags);
}

/*********************************************************************
 *		_wcreat (MSVCRT.@)
 */
int _wcreat(const MSVCRT_wchar_t *path, int flags)
{
  int usedFlags = (flags & MSVCRT__O_TEXT)| MSVCRT__O_CREAT| MSVCRT__O_WRONLY| MSVCRT__O_TRUNC;
  return _wopen(path, usedFlags);
}

/*********************************************************************
 *		_open_osfhandle (MSVCRT.@)
 */
int _open_osfhandle(long handle, int oflags)
{
  int fd;

  /* MSVCRT__O_RDONLY (0) always matches, so set the read flag
   * MFC's CStdioFile clears O_RDONLY (0)! if it wants to write to the
   * file, so set the write flag. It also only sets MSVCRT__O_TEXT if it wants
   * text - it never sets MSVCRT__O_BINARY.
   */
  /* FIXME: handle more flags */
  if (!(oflags & (MSVCRT__O_BINARY | MSVCRT__O_TEXT)) && (*__p__fmode() & MSVCRT__O_BINARY))
      oflags |= MSVCRT__O_BINARY;
  else
      oflags |= MSVCRT__O_TEXT;

  fd = msvcrt_alloc_fd((HANDLE)handle, split_oflags(oflags));
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n", handle, fd, oflags);
  return fd;
}

/*********************************************************************
 *		_rmtmp (MSVCRT.@)
 */
int _rmtmp(void)
{
  int num_removed = 0, i;

  LOCK_FILES();
  for (i = 3; i < MSVCRT_stream_idx; i++)
    if (MSVCRT_fstreams[i] && MSVCRT_fstreams[i]->_tmpfname)
    {
      MSVCRT_fclose(MSVCRT_fstreams[i]);
      num_removed++;
    }
  UNLOCK_FILES();

  if (num_removed)
    TRACE(":removed (%d) temp files\n",num_removed);
  return num_removed;
}

/*********************************************************************
 * (internal) remove_cr
 *
 *    Remove all \r inplace.
 * return the number of \r removed
 */
static unsigned int remove_cr(char *buf, unsigned int count)
{
    unsigned int i, j;

    for (i = 0; i < count; i++) if (buf[i] == '\r') break;
    for (j = i + 1; j < count; j++) if (buf[j] != '\r') buf[i++] = buf[j];
    return count - i;
}

/*********************************************************************
 *		_read (MSVCRT.@)
 */
int _read(int fd, void *buf, unsigned int count)
{
  DWORD num_read, all_read = 0;
  char *bufstart = buf;
  HANDLE hand = msvcrt_fdtoh(fd);

  /* Don't trace small reads, it gets *very* annoying */
  if (count > 4)
    TRACE(":fd (%d) handle (%p) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Reading single bytes in O_TEXT mode makes things slow
   * So read big chunks, then remove the \r in memory and try reading
   * the rest until the request is satisfied or EOF is met
   */
  while (all_read < count)
  {
      if (ReadFile(hand, bufstart+all_read, count - all_read, &num_read, NULL))
      {
          if (num_read != (count - all_read))
          {
              TRACE(":EOF\n");
              MSVCRT_fdesc[fd].wxflag |= WX_ATEOF;
              if (MSVCRT_fdesc[fd].wxflag & WX_TEXT)
                  num_read -= remove_cr(bufstart+all_read,num_read);
              all_read += num_read;
              if (count > 4)
                  TRACE("%s\n",debugstr_an(buf,all_read));
              return all_read;
          }
          if (MSVCRT_fdesc[fd].wxflag & WX_TEXT)
          {
              num_read -= remove_cr(bufstart+all_read,num_read);
          }
          all_read += num_read;
      }
      else
      {
          TRACE(":failed-last error (%ld)\n",GetLastError());
          return -1;
      }
  }

  if (count > 4)
      TRACE("(%lu), %s\n",all_read,debugstr_an(buf, all_read));
  return all_read;
}

/*********************************************************************
 *		_getw (MSVCRT.@)
 */
int MSVCRT__getw(MSVCRT_FILE* file)
{
  int i;
  switch (_read(file->_file, &i, sizeof(int)))
  {
  case 1: return i;
  case 0: file->_flag |= MSVCRT__IOEOF; break;
  default: file->_flag |= MSVCRT__IOERR; break;
  }
  return EOF;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int _setmode(int fd,int mode)
{
  int ret = MSVCRT_fdesc[fd].wxflag & WX_TEXT ? MSVCRT__O_TEXT : MSVCRT__O_BINARY;
  if (mode & (~(MSVCRT__O_TEXT|MSVCRT__O_BINARY)))
    FIXME("fd (%d) mode (0x%08x) unknown\n",fd,mode);
  if ((mode & MSVCRT__O_TEXT) == MSVCRT__O_TEXT)
    MSVCRT_fdesc[fd].wxflag |= WX_TEXT;
  else
    MSVCRT_fdesc[fd].wxflag &= ~WX_TEXT;
  return ret;
}

/*********************************************************************
 *		_stati64 (MSVCRT.@)
 */
int MSVCRT__stati64(const char* path, struct MSVCRT__stati64 * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%ld)\n",GetLastError());
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct MSVCRT__stati64));

  /* FIXME: rdev isn't drive num, despite what the docs say-what is it?
     Bon 011120: This FIXME seems incorrect
                 Also a letter as first char isn't enough to be classified
		 as a drive letter
  */
  if (isalpha(*path)&& (*(path+1)==':'))
    buf->st_dev = buf->st_rdev = toupper(*path) - 'A'; /* drive num */
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

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
  TRACE("%d %d 0x%08lx%08lx %ld %ld %ld\n", buf->st_mode,buf->st_nlink,
        (long)(buf->st_size >> 32),(long)buf->st_size,
        buf->st_atime,buf->st_mtime, buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_stat (MSVCRT.@)
 */
int MSVCRT__stat(const char* path, struct MSVCRT__stat * buf)
{ int ret;
  struct MSVCRT__stati64 bufi64;

  ret = MSVCRT__stati64( path, &bufi64);
  if (!ret)
      msvcrt_cp_from_stati64(&bufi64, buf);
  return ret;
}

/*********************************************************************
 *		_wstati64 (MSVCRT.@)
 */
int MSVCRT__wstati64(const MSVCRT_wchar_t* path, struct MSVCRT__stati64 * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = ALL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",debugstr_w(path),buf);

  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%ld)\n",GetLastError());
      msvcrt_set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct MSVCRT__stat));

  /* FIXME: rdev isn't drive num, despite what the docs says-what is it? */
  if (MSVCRT_iswalpha(*path))
    buf->st_dev = buf->st_rdev = toupperW(*path - 'A'); /* drive num */
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

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
  TRACE("%d %d 0x%08lx%08lx %ld %ld %ld\n", buf->st_mode,buf->st_nlink,
        (long)(buf->st_size >> 32),(long)buf->st_size,
        buf->st_atime,buf->st_mtime, buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_wstat (MSVCRT.@)
 */
int MSVCRT__wstat(const MSVCRT_wchar_t* path, struct MSVCRT__stat * buf)
{
  int ret;
  struct MSVCRT__stati64 bufi64;

  ret = MSVCRT__wstati64( path, &bufi64 );
  if (!ret) msvcrt_cp_from_stati64(&bufi64, buf);
  return ret;
}

/*********************************************************************
 *		_tell (MSVCRT.@)
 */
long _tell(int fd)
{
  return _lseek(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_telli64 (MSVCRT.@)
 */
__int64 _telli64(int fd)
{
  return _lseeki64(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_tempnam (MSVCRT.@)
 */
char *_tempnam(const char *dir, const char *prefix)
{
  char tmpbuf[MAX_PATH];
  const char *tmp_dir = MSVCRT_getenv("TMP");

  if (tmp_dir) dir = tmp_dir;

  TRACE("dir (%s) prefix (%s)\n",dir,prefix);
  if (GetTempFileNameA(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",tmpbuf);
    DeleteFileA(tmpbuf);
    return _strdup(tmpbuf);
  }
  TRACE("failed (%ld)\n",GetLastError());
  return NULL;
}

/*********************************************************************
 *		_wtempnam (MSVCRT.@)
 */
MSVCRT_wchar_t *_wtempnam(const MSVCRT_wchar_t *dir, const MSVCRT_wchar_t *prefix)
{
  MSVCRT_wchar_t tmpbuf[MAX_PATH];

  TRACE("dir (%s) prefix (%s)\n",debugstr_w(dir),debugstr_w(prefix));
  if (GetTempFileNameW(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",debugstr_w(tmpbuf));
    DeleteFileW(tmpbuf);
    return _wcsdup(tmpbuf);
  }
  TRACE("failed (%ld)\n",GetLastError());
  return NULL;
}

/*********************************************************************
 *		_umask (MSVCRT.@)
 */
int _umask(int umask)
{
  int old_umask = MSVCRT_umask;
  TRACE("(%d)\n",umask);
  MSVCRT_umask = umask;
  return old_umask;
}

/*********************************************************************
 *		_utime (MSVCRT.@)
 */
int _utime(const char* path, struct MSVCRT__utimbuf *t)
{
  int fd = _open(path, MSVCRT__O_WRONLY | MSVCRT__O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime(fd, t);
    _close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_wutime (MSVCRT.@)
 */
int _wutime(const MSVCRT_wchar_t* path, struct MSVCRT__utimbuf *t)
{
  int fd = _wopen(path, MSVCRT__O_WRONLY | MSVCRT__O_BINARY);

  if (fd > 0)
  {
    int retVal = _futime(fd, t);
    _close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_write (MSVCRT.@)
 */
int _write(int fd, const void* buf, unsigned int count)
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
  if (MSVCRT_fdesc[fd].wxflag & WX_APPEND)
    _lseek(fd, 0, FILE_END);

  if (!(MSVCRT_fdesc[fd].wxflag & WX_TEXT))
    {
      if (WriteFile(hand, buf, count, &num_written, NULL)
	  &&  (num_written == count))
	return num_written;
      TRACE("WriteFile (fd %d, hand %p) failed-last error (%ld)\n", fd,
       hand, GetLastError());
      *MSVCRT__errno() = MSVCRT_ENOSPC;
    }
  else
  {
      unsigned int i, j, nr_lf;
      char *s =(char*)buf, *buf_start=(char*)buf, *p;
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
          if ((p = MSVCRT_malloc(count + nr_lf)))
          {
              for(s=(char*)buf, i=0, j=0; i<count; i++)
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
              p = (char*)buf;
          }
      }
      else
          p = (char*)buf;

      if ((WriteFile(hand, p, count+nr_lf, &num_written, NULL) == 0 ) || (num_written != count+nr_lf))
      {
          TRACE("WriteFile (fd %d, hand %p) failed-last error (%ld), num_written %ld\n",
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
int MSVCRT__putw(int val, MSVCRT_FILE* file)
{
  int len;
  len = _write(file->_file, &val, sizeof(val));
  if (len == sizeof(val)) return val;
  file->_flag |= MSVCRT__IOERR;
  return MSVCRT_EOF;
}

/*********************************************************************
 *		fclose (MSVCRT.@)
 */
int MSVCRT_fclose(MSVCRT_FILE* file)
{
  int r, flag;

  flag = file->_flag;
  if (file->_tmpfname)
  {
    MSVCRT_free(file->_tmpfname);
    file->_tmpfname = NULL;
  }
  /* flush stdio buffers */
  if(file->_flag & MSVCRT__IOWRT)
      MSVCRT_fflush(file);
  if(file->_flag & MSVCRT__IOMYBUF)
      MSVCRT_free(file->_base);

  r=_close(file->_file);

  file->_flag = 0;

  return ((r == -1) || (flag & MSVCRT__IOERR) ? MSVCRT_EOF : 0);
}

/*********************************************************************
 *		feof (MSVCRT.@)
 */
int MSVCRT_feof(MSVCRT_FILE* file)
{
  return file->_flag & MSVCRT__IOEOF;
}

/*********************************************************************
 *		ferror (MSVCRT.@)
 */
int MSVCRT_ferror(MSVCRT_FILE* file)
{
  return file->_flag & MSVCRT__IOERR;
}

/*********************************************************************
 *		_filbuf (MSVCRT.@)
 */
int MSVCRT__filbuf(MSVCRT_FILE* file)
{
  /* Allocate buffer if needed */
  if(file->_bufsiz == 0 && !(file->_flag & MSVCRT__IONBF) ) {
	msvcrt_alloc_buffer(file);
  }
  if(!(file->_flag & MSVCRT__IOREAD)) {
	if(file->_flag & MSVCRT__IORW) {
		file->_flag |= MSVCRT__IOREAD;
	} else {
		return MSVCRT_EOF;
	}
  }
  if(file->_flag & MSVCRT__IONBF) {
	unsigned char c;
        int r;
  	if ((r = _read(file->_file,&c,1)) != 1) {
            file->_flag |= (r == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
            return MSVCRT_EOF;
	}
  	return c;
  } else {
	file->_cnt = _read(file->_file, file->_base, file->_bufsiz);
	if(file->_cnt<=0) {
            file->_flag |= (file->_cnt == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
            file->_cnt = 0;
            return MSVCRT_EOF;
	}
	file->_cnt--;
	file->_ptr = file->_base+1;
	return *(unsigned char *)file->_base;
  }
}

/*********************************************************************
 *		fgetc (MSVCRT.@)
 */
int MSVCRT_fgetc(MSVCRT_FILE* file)
{
  if (file->_cnt>0) {
	file->_cnt--;
	return *(unsigned char *)file->_ptr++;
  } else {
	return MSVCRT__filbuf(file);
  }
}

/*********************************************************************
 *		_fgetchar (MSVCRT.@)
 */
int _fgetchar(void)
{
  return MSVCRT_fgetc(MSVCRT_stdin);
}

/*********************************************************************
 *		fgets (MSVCRT.@)
 */
char *MSVCRT_fgets(char *s, int size, MSVCRT_FILE* file)
{
  int    cc = MSVCRT_EOF;
  char * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
	file,file->_file,s,size);

  while ((size >1) && (cc = MSVCRT_fgetc(file)) != MSVCRT_EOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == MSVCRT_EOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    return NULL;
  }
  if ((cc != MSVCRT_EOF) && (size > 1))
    *s++ = cc;
  *s = '\0';
  TRACE(":got %s\n", debugstr_a(buf_start));
  return buf_start;
}

/*********************************************************************
 *		fgetwc (MSVCRT.@)
 *
 * In MSVCRT__O_TEXT mode, multibyte characters are read from the file, dropping
 * the CR from CR/LF combinations
 */
MSVCRT_wint_t MSVCRT_fgetwc(MSVCRT_FILE* file)
{
  char c;

  if (!(MSVCRT_fdesc[file->_file].wxflag & WX_TEXT))
    {
      MSVCRT_wchar_t wc;
      int r;
      if ((r = _read(file->_file, &wc, sizeof(wc))) != sizeof(wc))
      {
          file->_flag |= (r == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
          return MSVCRT_WEOF;
      }
      return wc;
    }
  c = MSVCRT_fgetc(file);
  if ((*__p___mb_cur_max() > 1) && MSVCRT_isleadbyte(c))
    {
      FIXME("Treat Multibyte characters\n");
    }
  if (c == MSVCRT_EOF)
    return MSVCRT_WEOF;
  else
    return (MSVCRT_wint_t)c;
}

/*********************************************************************
 *		getwc (MSVCRT.@)
 */
MSVCRT_wint_t MSVCRT_getwc(MSVCRT_FILE* file)
{
  return MSVCRT_fgetwc(file);
}

/*********************************************************************
 *		_fgetwchar (MSVCRT.@)
 */
MSVCRT_wint_t _fgetwchar(void)
{
  return MSVCRT_fgetwc(MSVCRT_stdin);
}

/*********************************************************************
 *		getwchar (MSVCRT.@)
 */
MSVCRT_wint_t MSVCRT_getwchar(void)
{
  return _fgetwchar();
}

/*********************************************************************
 *              fgetws (MSVCRT.@)
 */
MSVCRT_wchar_t *MSVCRT_fgetws(MSVCRT_wchar_t *s, int size, MSVCRT_FILE* file)
{
  int    cc = MSVCRT_WEOF;
  MSVCRT_wchar_t * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
        file,file->_file,s,size);

  while ((size >1) && (cc = MSVCRT_fgetwc(file)) != MSVCRT_WEOF && cc != '\n')
    {
      *s++ = (char)cc;
      size --;
    }
  if ((cc == MSVCRT_WEOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    return NULL;
  }
  if ((cc != MSVCRT_WEOF) && (size > 1))
    *s++ = cc;
  *s = 0;
  TRACE(":got %s\n", debugstr_w(buf_start));
  return buf_start;
}

/*********************************************************************
 *		fwrite (MSVCRT.@)
 */
MSVCRT_size_t MSVCRT_fwrite(const void *ptr, MSVCRT_size_t size, MSVCRT_size_t nmemb, MSVCRT_FILE* file)
{
  MSVCRT_size_t wrcnt=size * nmemb;
  int written = 0;
  if (size == 0)
      return 0;
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
	} else
		return 0;
  }
  if(wrcnt) {
	/* Flush buffer */
  	int res=msvcrt_flush_buffer(file);
	if(!res) {
		int pwritten = _write(file->_file, ptr, wrcnt);
  		if (pwritten <= 0)
                {
                    file->_flag |= MSVCRT__IOERR;
                    pwritten=0;
                }
		written += pwritten;
	}
  }
  return written / size;
}

/*********************************************************************
 *		fputwc (MSVCRT.@)
 */
MSVCRT_wint_t MSVCRT_fputwc(MSVCRT_wint_t wc, MSVCRT_FILE* file)
{
  MSVCRT_wchar_t mwc=wc;
  if (MSVCRT_fwrite( &mwc, sizeof(mwc), 1, file) != 1)
    return MSVCRT_WEOF;
  return wc;
}

/*********************************************************************
 *		_fputwchar (MSVCRT.@)
 */
MSVCRT_wint_t _fputwchar(MSVCRT_wint_t wc)
{
  return MSVCRT_fputwc(wc, MSVCRT_stdout);
}

/*********************************************************************
 *		fopen (MSVCRT.@)
 */
MSVCRT_FILE* MSVCRT_fopen(const char *path, const char *mode)
{
  MSVCRT_FILE* file;
  int open_flags, stream_flags, fd;

  TRACE("(%s,%s)\n",path,mode);

  /* map mode string to open() flags. "man fopen" for possibilities. */
  if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1)
      return NULL;

  LOCK_FILES();
  fd = _open(path, open_flags, MSVCRT__S_IREAD | MSVCRT__S_IWRITE);
  if (fd < 0)
    file = NULL;
  else if ((file = msvcrt_alloc_fp()) && msvcrt_init_fp(file, fd, stream_flags)
   != -1)
    TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,mode,file);
  else if (file)
  {
    file->_flag = 0;
    file = NULL;
  }

  TRACE(":got (%p)\n",file);
  if (fd >= 0 && !file)
    _close(fd);
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *		_wfopen (MSVCRT.@)
 */
MSVCRT_FILE *MSVCRT__wfopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode)
{
  const unsigned int plen = strlenW(path), mlen = strlenW(mode);
  char *patha = MSVCRT_calloc(plen + 1, 1);
  char *modea = MSVCRT_calloc(mlen + 1, 1);

  TRACE("(%s,%s)\n",debugstr_w(path),debugstr_w(mode));

  if (patha && modea &&
      WideCharToMultiByte(CP_ACP,0,path,plen,patha,plen,NULL,NULL) &&
      WideCharToMultiByte(CP_ACP,0,mode,mlen,modea,mlen,NULL,NULL))
  {
    MSVCRT_FILE *retval = MSVCRT_fopen(patha,modea);
    MSVCRT_free(patha);
    MSVCRT_free(modea);
    return retval;
  }

  msvcrt_set_errno(GetLastError());
  return NULL;
}

/*********************************************************************
 *		_fsopen (MSVCRT.@)
 */
MSVCRT_FILE*  MSVCRT__fsopen(const char *path, const char *mode, int share)
{
  FIXME(":(%s,%s,%d),ignoring share mode!\n",path,mode,share);
  return MSVCRT_fopen(path,mode);
}

/*********************************************************************
 *		_wfsopen (MSVCRT.@)
 */
MSVCRT_FILE*  MSVCRT__wfsopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode, int share)
{
  FIXME(":(%s,%s,%d),ignoring share mode!\n",
        debugstr_w(path),debugstr_w(mode),share);
  return MSVCRT__wfopen(path,mode);
}

/* MSVCRT_fputc calls MSVCRT__flsbuf which calls MSVCRT_fputc */
int MSVCRT__flsbuf(int c, MSVCRT_FILE* file);

/*********************************************************************
 *		fputc (MSVCRT.@)
 */
int MSVCRT_fputc(int c, MSVCRT_FILE* file)
{
  if(file->_cnt>0) {
    *file->_ptr++=c;
    file->_cnt--;
    if (c == '\n')
    {
      int res = msvcrt_flush_buffer(file);
      return res ? res : c;
    }
    else
      return c;
  } else {
    return MSVCRT__flsbuf(c, file);
  }
}

/*********************************************************************
 *		_flsbuf (MSVCRT.@)
 */
int MSVCRT__flsbuf(int c, MSVCRT_FILE* file)
{
  /* Flush output buffer */
  if(file->_bufsiz == 0 && !(file->_flag & MSVCRT__IONBF)) {
	msvcrt_alloc_buffer(file);
  }
  if(!(file->_flag & MSVCRT__IOWRT)) {
	if(file->_flag & MSVCRT__IORW) {
		file->_flag |= MSVCRT__IOWRT;
	} else {
		return MSVCRT_EOF;
	}
  }
  if(file->_bufsiz) {
        int res=msvcrt_flush_buffer(file);
	return res?res : MSVCRT_fputc(c, file);
  } else {
	unsigned char cc=c;
        int len;
  	len = _write(file->_file, &cc, 1);
        if (len == 1) return c;
        file->_flag |= MSVCRT__IOERR;
        return MSVCRT_EOF;
  }
}

/*********************************************************************
 *		_fputchar (MSVCRT.@)
 */
int _fputchar(int c)
{
  return MSVCRT_fputc(c, MSVCRT_stdout);
}

/*********************************************************************
 *		fread (MSVCRT.@)
 */
MSVCRT_size_t MSVCRT_fread(void *ptr, MSVCRT_size_t size, MSVCRT_size_t nmemb, MSVCRT_FILE* file)
{ MSVCRT_size_t rcnt=size * nmemb;
  MSVCRT_size_t read=0;
  int pread=0;

  if(!rcnt)
	return 0;

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
	} else
            return 0;
  }
  if(rcnt)
  {
    pread = _read(file->_file,ptr, rcnt);
    /* expose feof condition in the flags
     * MFC tests file->_flag for feof, and doesn't not call feof())
     */
    if ( MSVCRT_fdesc[file->_file].wxflag & WX_ATEOF)
        file->_flag |= MSVCRT__IOEOF;
    else if (pread == -1)
    {
        file->_flag |= MSVCRT__IOERR;
        pread = 0;
    }
  }
  read+=pread;
  return read / size;
}

/*********************************************************************
 *		freopen (MSVCRT.@)
 *
 */
MSVCRT_FILE* MSVCRT_freopen(const char *path, const char *mode,MSVCRT_FILE* file)
{
  int open_flags, stream_flags, fd;

  TRACE(":path (%p) mode (%s) file (%p) fd (%d)\n",path,mode,file,file->_file);

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
      fd = _open(path, open_flags, MSVCRT__S_IREAD | MSVCRT__S_IWRITE);
      if (fd < 0)
        file = NULL;
      else if (msvcrt_init_fp(file, fd, stream_flags) == -1)
      {
          file->_flag = 0;
          WARN(":failed-last error (%ld)\n",GetLastError());
          msvcrt_set_errno(GetLastError());
          file = NULL;
      }
    }
  }
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *		fsetpos (MSVCRT.@)
 */
int MSVCRT_fsetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
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

  return (_lseeki64(file->_file,*pos,SEEK_SET) == -1) ? -1 : 0;
}

/*********************************************************************
 *		ftell (MSVCRT.@)
 */
LONG MSVCRT_ftell(MSVCRT_FILE* file)
{
  int off=0;
  long pos;
  if(file->_bufsiz)  {
	if( file->_flag & MSVCRT__IOWRT ) {
		off = file->_ptr - file->_base;
	} else {
		off = -file->_cnt;
	}
  }
  pos = _tell(file->_file);
  if(pos == -1) return pos;
  return off + pos;
}

/*********************************************************************
 *		fgetpos (MSVCRT.@)
 */
int MSVCRT_fgetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
  /* This code has been lifted form the MSVCRT_ftell function */
  int off=0;

  *pos = _lseeki64(file->_file,0,SEEK_CUR);

  if (*pos == -1) return -1;
  
  if(file->_bufsiz)  {
	if( file->_flag & MSVCRT__IOWRT ) {
		off = file->_ptr - file->_base;
	} else {
		off = -file->_cnt;
	}
  }
  *pos += off;
  
  return 0;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int MSVCRT_fputs(const char *s, MSVCRT_FILE* file)
{
    size_t i, len = strlen(s);
    if (!(MSVCRT_fdesc[file->_file].wxflag & WX_TEXT))
      return MSVCRT_fwrite(s,sizeof(*s),len,file) == len ? 0 : MSVCRT_EOF;
    for (i=0; i<len; i++)
      if (MSVCRT_fputc(s[i], file) == MSVCRT_EOF) 
	return MSVCRT_EOF;
    return 0;
}

/*********************************************************************
 *		fputws (MSVCRT.@)
 */
int MSVCRT_fputws(const MSVCRT_wchar_t *s, MSVCRT_FILE* file)
{
    size_t i, len = strlenW(s);
    if (!(MSVCRT_fdesc[file->_file].wxflag & WX_TEXT))
      return MSVCRT_fwrite(s,sizeof(*s),len,file) == len ? 0 : MSVCRT_EOF;
    for (i=0; i<len; i++)
      {
	if ((s[i] == L'\n') && (MSVCRT_fputc('\r', file) == MSVCRT_EOF))
	  return MSVCRT_WEOF;
	if (MSVCRT_fputwc(s[i], file) == MSVCRT_WEOF)
	  return MSVCRT_WEOF; 
      }
    return 0;
}

/*********************************************************************
 *		getchar (MSVCRT.@)
 */
int MSVCRT_getchar(void)
{
  return MSVCRT_fgetc(MSVCRT_stdin);
}

/*********************************************************************
 *		getc (MSVCRT.@)
 */
int MSVCRT_getc(MSVCRT_FILE* file)
{
  return MSVCRT_fgetc(file);
}

/*********************************************************************
 *		gets (MSVCRT.@)
 */
char *MSVCRT_gets(char *buf)
{
  int    cc;
  char * buf_start = buf;

  for(cc = MSVCRT_fgetc(MSVCRT_stdin); cc != MSVCRT_EOF && cc != '\n';
      cc = MSVCRT_fgetc(MSVCRT_stdin))
  if(cc != '\r') *buf++ = (char)cc;

  *buf = '\0';

  TRACE("got '%s'\n", buf_start);
  return buf_start;
}

/*********************************************************************
 *		_getws (MSVCRT.@)
 */
MSVCRT_wchar_t* MSVCRT__getws(MSVCRT_wchar_t* buf)
{
    MSVCRT_wint_t cc;
    MSVCRT_wchar_t* ws = buf;

    for (cc = MSVCRT_fgetwc(MSVCRT_stdin); cc != MSVCRT_WEOF && cc != '\n';
         cc = MSVCRT_fgetwc(MSVCRT_stdin))
    {
        if (cc != '\r')
            *buf++ = (MSVCRT_wchar_t)cc;
    }
    *buf = '\0';

    TRACE("got '%s'\n", debugstr_w(ws));
    return ws;
}

/*********************************************************************
 *		putc (MSVCRT.@)
 */
int MSVCRT_putc(int c, MSVCRT_FILE* file)
{
  return MSVCRT_fputc(c, file);
}

/*********************************************************************
 *		putchar (MSVCRT.@)
 */
int MSVCRT_putchar(int c)
{
  return MSVCRT_fputc(c, MSVCRT_stdout);
}

/*********************************************************************
 *		puts (MSVCRT.@)
 */
int MSVCRT_puts(const char *s)
{
    size_t len = strlen(s);
    if (MSVCRT_fwrite(s,sizeof(*s),len,MSVCRT_stdout) != len) return MSVCRT_EOF;
    return MSVCRT_fwrite("\n",1,1,MSVCRT_stdout) == 1 ? 0 : MSVCRT_EOF;
}

/*********************************************************************
 *		_putws (MSVCRT.@)
 */
int _putws(const MSVCRT_wchar_t *s)
{
    static const MSVCRT_wchar_t nl = '\n';
    size_t len = strlenW(s);
    if (MSVCRT_fwrite(s,sizeof(*s),len,MSVCRT_stdout) != len) return MSVCRT_EOF;
    return MSVCRT_fwrite(&nl,sizeof(nl),1,MSVCRT_stdout) == 1 ? 0 : MSVCRT_EOF;
}

/*********************************************************************
 *		remove (MSVCRT.@)
 */
int MSVCRT_remove(const char *path)
{
  TRACE("(%s)\n",path);
  if (DeleteFileA(path))
    return 0;
  TRACE(":failed (%ld)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wremove (MSVCRT.@)
 */
int _wremove(const MSVCRT_wchar_t *path)
{
  TRACE("(%s)\n",debugstr_w(path));
  if (DeleteFileW(path))
    return 0;
  TRACE(":failed (%ld)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		rename (MSVCRT.@)
 */
int MSVCRT_rename(const char *oldpath,const char *newpath)
{
  TRACE(":from %s to %s\n",oldpath,newpath);
  if (MoveFileExA(oldpath, newpath, MOVEFILE_COPY_ALLOWED))
    return 0;
  TRACE(":failed (%ld)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wrename (MSVCRT.@)
 */
int _wrename(const MSVCRT_wchar_t *oldpath,const MSVCRT_wchar_t *newpath)
{
  TRACE(":from %s to %s\n",debugstr_w(oldpath),debugstr_w(newpath));
  if (MoveFileExW(oldpath, newpath, MOVEFILE_COPY_ALLOWED))
    return 0;
  TRACE(":failed (%ld)\n",GetLastError());
  msvcrt_set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		setvbuf (MSVCRT.@)
 */
int MSVCRT_setvbuf(MSVCRT_FILE* file, char *buf, int mode, MSVCRT_size_t size)
{
  /* TODO: Check if file busy */
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
  return 0;
}

/*********************************************************************
 *		setbuf (MSVCRT.@)
 */
void MSVCRT_setbuf(MSVCRT_FILE* file, char *buf)
{
  MSVCRT_setvbuf(file, buf, buf ? MSVCRT__IOFBF : MSVCRT__IONBF, MSVCRT_BUFSIZ);
}

/*********************************************************************
 *		tmpnam (MSVCRT.@)
 */
char *MSVCRT_tmpnam(char *s)
{
  static int unique;
  char tmpstr[16];
  char *p;
  int count;
  if (s == 0)
    s = MSVCRT_tmpname;
  msvcrt_int_to_base32(GetCurrentProcessId(), tmpstr);
  p = s + sprintf(s, "\\s%s.", tmpstr);
  for (count = 0; count < MSVCRT_TMP_MAX; count++)
  {
    msvcrt_int_to_base32(unique++, tmpstr);
    strcpy(p, tmpstr);
    if (GetFileAttributesA(s) == INVALID_FILE_ATTRIBUTES &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      break;
  }
  return s;
}

/*********************************************************************
 *		tmpfile (MSVCRT.@)
 */
MSVCRT_FILE* MSVCRT_tmpfile(void)
{
  char *filename = MSVCRT_tmpnam(NULL);
  int fd;
  MSVCRT_FILE* file = NULL;

  LOCK_FILES();
  fd = _open(filename, MSVCRT__O_CREAT | MSVCRT__O_BINARY | MSVCRT__O_RDWR | MSVCRT__O_TEMPORARY);
  if (fd != -1 && (file = msvcrt_alloc_fp()))
  {
    if (msvcrt_init_fp(file, fd, MSVCRT__O_RDWR) == -1)
    {
        file->_flag = 0;
        file = NULL;
    }
    else file->_tmpfname = _strdup(filename);
  }
  UNLOCK_FILES();
  return file;
}

/*********************************************************************
 *		vfprintf (MSVCRT.@)
 */
int MSVCRT_vfprintf(MSVCRT_FILE* file, const char *format, va_list valist)
{
  char buf[2048], *mem = buf;
  int written, resize = sizeof(buf), retval;
  /* There are two conventions for vsnprintf failing:
   * Return -1 if we truncated, or
   * Return the number of bytes that would have been written
   * The code below handles both cases
   */
  while ((written = MSVCRT_vsnprintf(mem, resize, format, valist)) == -1 ||
          written > resize)
  {
    resize = (written == -1 ? resize * 2 : written + 1);
    if (mem != buf)
      MSVCRT_free (mem);
    if (!(mem = (char *)MSVCRT_malloc(resize)))
      return MSVCRT_EOF;
  }
  retval = MSVCRT_fwrite(mem, sizeof(*mem), written, file);
  if (mem != buf)
    MSVCRT_free (mem);
  return retval;
}

/*********************************************************************
 *		vfwprintf (MSVCRT.@)
 * FIXME:
 * Is final char included in written (then resize is too big) or not
 * (then we must test for equality too)?
 */
int MSVCRT_vfwprintf(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, va_list valist)
{
  MSVCRT_wchar_t buf[2048], *mem = buf;
  int written, resize = sizeof(buf) / sizeof(MSVCRT_wchar_t), retval;
  /* See vfprintf comments */
  while ((written = MSVCRT_vsnwprintf(mem, resize, format, valist)) == -1 ||
          written > resize)
  {
    resize = (written == -1 ? resize * 2 : written + sizeof(MSVCRT_wchar_t));
    if (mem != buf)
      MSVCRT_free (mem);
    if (!(mem = (MSVCRT_wchar_t *)MSVCRT_malloc(resize*sizeof(*mem))))
      return MSVCRT_EOF;
  }
  retval = MSVCRT_fwrite(mem, sizeof(*mem), written, file);
  if (mem != buf)
    MSVCRT_free (mem);
  return retval;
}

/*********************************************************************
 *		vprintf (MSVCRT.@)
 */
int MSVCRT_vprintf(const char *format, va_list valist)
{
  return MSVCRT_vfprintf(MSVCRT_stdout,format,valist);
}

/*********************************************************************
 *		vwprintf (MSVCRT.@)
 */
int MSVCRT_vwprintf(const MSVCRT_wchar_t *format, va_list valist)
{
  return MSVCRT_vfwprintf(MSVCRT_stdout,format,valist);
}

/*********************************************************************
 *		fprintf (MSVCRT.@)
 */
int MSVCRT_fprintf(MSVCRT_FILE* file, const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = MSVCRT_vfprintf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fwprintf (MSVCRT.@)
 */
int MSVCRT_fwprintf(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = MSVCRT_vfwprintf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		printf (MSVCRT.@)
 */
int MSVCRT_printf(const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = MSVCRT_vfprintf(MSVCRT_stdout, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		ungetc (MSVCRT.@)
 */
int MSVCRT_ungetc(int c, MSVCRT_FILE * file)
{
	if (c == MSVCRT_EOF)
		return MSVCRT_EOF;
	if(file->_bufsiz == 0 && !(file->_flag & MSVCRT__IONBF)) {
		msvcrt_alloc_buffer(file);
		file->_ptr++;
	}
	if(file->_ptr>file->_base) {
		file->_ptr--;
		*file->_ptr=c;
		file->_cnt++;
		MSVCRT_clearerr(file);
		return c;
	}
	return MSVCRT_EOF;
}

/*********************************************************************
 *              ungetwc (MSVCRT.@)
 */
MSVCRT_wint_t MSVCRT_ungetwc(MSVCRT_wint_t wc, MSVCRT_FILE * file)
{
	MSVCRT_wchar_t mwc = wc;
	char * pp = (char *)&mwc;
	int i;
	for(i=sizeof(MSVCRT_wchar_t)-1;i>=0;i--) {
		if(pp[i] != MSVCRT_ungetc(pp[i],file))
			return MSVCRT_WEOF;
	}
	return mwc;
}

/*********************************************************************
 *		wprintf (MSVCRT.@)
 */
int MSVCRT_wprintf(const MSVCRT_wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = MSVCRT_vwprintf(format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		__pioinfo (MSVCRT.@)
 * FIXME: see MSVCRT_MAX_FILES define.
 */
ioinfo * MSVCRT___pioinfo[] = { /* array of pointers to ioinfo arrays [64] */
    &MSVCRT_fdesc[0 * 64], &MSVCRT_fdesc[1 * 64], &MSVCRT_fdesc[2 * 64],
    &MSVCRT_fdesc[3 * 64], &MSVCRT_fdesc[4 * 64], &MSVCRT_fdesc[5 * 64],
    &MSVCRT_fdesc[6 * 64], &MSVCRT_fdesc[7 * 64], &MSVCRT_fdesc[8 * 64],
    &MSVCRT_fdesc[9 * 64], &MSVCRT_fdesc[10 * 64], &MSVCRT_fdesc[11 * 64],
    &MSVCRT_fdesc[12 * 64], &MSVCRT_fdesc[13 * 64], &MSVCRT_fdesc[14 * 64],
    &MSVCRT_fdesc[15 * 64], &MSVCRT_fdesc[16 * 64], &MSVCRT_fdesc[17 * 64],
    &MSVCRT_fdesc[18 * 64], &MSVCRT_fdesc[19 * 64], &MSVCRT_fdesc[20 * 64],
    &MSVCRT_fdesc[21 * 64], &MSVCRT_fdesc[22 * 64], &MSVCRT_fdesc[23 * 64],
    &MSVCRT_fdesc[24 * 64], &MSVCRT_fdesc[25 * 64], &MSVCRT_fdesc[26 * 64],
    &MSVCRT_fdesc[27 * 64], &MSVCRT_fdesc[28 * 64], &MSVCRT_fdesc[29 * 64],
    &MSVCRT_fdesc[30 * 64], &MSVCRT_fdesc[31 * 64]
} ;

/*********************************************************************
 *		__badioinfo (MSVCRT.@)
 */
ioinfo MSVCRT___badioinfo = { INVALID_HANDLE_VALUE, WX_TEXT };
