/*
 * msvcrt.dll file functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "msvcrt.h"
#include "msvcrt/errno.h"

#include "wine/unicode.h"
#include "msvcrt/direct.h"
#include "msvcrt/fcntl.h"
#include "msvcrt/io.h"
#include "msvcrt/sys/locking.h"
#include "msvcrt/stdio.h"
#include "msvcrt/stdlib.h"
#include "msvcrt/string.h"
#include "msvcrt/sys/stat.h"
#include "msvcrt/sys/utime.h"
#include "msvcrt/time.h"
#include "msvcrt/share.h"
#include "msvcrt/wctype.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* for stat mode, permissions apply to all,owner and group */
#define MSVCRT_S_IREAD  (_S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6))
#define MSVCRT_S_IWRITE (_S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6))
#define MSVCRT_S_IEXEC  (_S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6))

/* _access() bit flags FIXME: incomplete */
#define MSVCRT_W_OK      0x02


/* FIXME: Make this dynamic */
#define MSVCRT_MAX_FILES 257

HANDLE MSVCRT_handles[MSVCRT_MAX_FILES];
MSVCRT_FILE* MSVCRT_files[MSVCRT_MAX_FILES];
int  MSVCRT_flags[MSVCRT_MAX_FILES];
char *MSVCRT_tempfiles[MSVCRT_MAX_FILES];
MSVCRT_FILE MSVCRT__iob[3];
#define MSVCRT_stdin       (MSVCRT__iob+STDIN_FILENO)
#define MSVCRT_stdout      (MSVCRT__iob+STDOUT_FILENO)
#define MSVCRT_stderr      (MSVCRT__iob+STDERR_FILENO)

static int MSVCRT_fdstart = 3; /* first unallocated fd */
static int MSVCRT_fdend = 3; /* highest allocated fd */

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

extern CRITICAL_SECTION MSVCRT_file_cs;
#define LOCK_FILES     EnterCriticalSection(&MSVCRT_file_cs)
#define UNLOCK_FILES   LeaveCriticalSection(&MSVCRT_file_cs)

static void msvcrt_cp_from_stati64(const struct _stati64 *bufi64, struct _stat *buf)
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

/* INTERNAL: Get the HANDLE for a fd */
static HANDLE msvcrt_fdtoh(int fd)
{
  if (fd < 0 || fd >= MSVCRT_fdend ||
      MSVCRT_handles[fd] == INVALID_HANDLE_VALUE)
  {
    WARN(":fd (%d) - no handle!\n",fd);
    *__doserrno() = 0;
    *MSVCRT__errno() = MSVCRT_EBADF;
   return INVALID_HANDLE_VALUE;
  }
  return MSVCRT_handles[fd];
}

/* INTERNAL: free a file entry fd */
static void msvcrt_free_fd(int fd)
{
  MSVCRT_handles[fd] = INVALID_HANDLE_VALUE;
  MSVCRT_files[fd] = 0;
  MSVCRT_flags[fd] = 0;
  TRACE(":fd (%d) freed\n",fd);
  if (fd < 3)
    return; /* dont use 0,1,2 for user files */
  if (fd == MSVCRT_fdend - 1)
    MSVCRT_fdend--;
  if (fd < MSVCRT_fdstart)
    MSVCRT_fdstart = fd;
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE */
static int msvcrt_alloc_fd(HANDLE hand, int flag)
{
  int fd = MSVCRT_fdstart;

  TRACE(":handle (%p) allocating fd (%d)\n",hand,fd);
  if (fd >= MSVCRT_MAX_FILES)
  {
    WARN(":files exhausted!\n");
    return -1;
  }
  MSVCRT_handles[fd] = hand;
  MSVCRT_flags[fd] = flag;

  /* locate next free slot */
  if (fd == MSVCRT_fdend)
    MSVCRT_fdstart = ++MSVCRT_fdend;
  else
    while(MSVCRT_fdstart < MSVCRT_fdend &&
	  MSVCRT_handles[MSVCRT_fdstart] != INVALID_HANDLE_VALUE)
      MSVCRT_fdstart++;

  return fd;
}

/* INTERNAL: Allocate a FILE* for an fd slot
 * This is done lazily to avoid memory wastage for low level open/write
 * usage when a FILE* is not requested (but may be later).
 */
static MSVCRT_FILE* msvcrt_alloc_fp(int fd)
{
  TRACE(":fd (%d) allocating FILE*\n",fd);
  if (fd < 0 || fd >= MSVCRT_fdend ||
      MSVCRT_handles[fd] == INVALID_HANDLE_VALUE)
  {
    WARN(":invalid fd %d\n",fd);
    *__doserrno() = 0;
    *MSVCRT__errno() = MSVCRT_EBADF;
    return NULL;
  }
  if (!MSVCRT_files[fd])
  {
    if ((MSVCRT_files[fd] = MSVCRT_calloc(sizeof(MSVCRT_FILE),1)))
    {
      MSVCRT_files[fd]->_file = fd;
      MSVCRT_files[fd]->_flag = MSVCRT_flags[fd];
      MSVCRT_files[fd]->_flag &= ~MSVCRT__IOAPPEND; /* mask out, see above */
    }
  }
  TRACE(":got FILE* (%p)\n",MSVCRT_files[fd]);
  return MSVCRT_files[fd];
}


/* INTERNAL: Set up stdin, stderr and stdout */
void msvcrt_init_io(void)
{
  int i;
  memset(MSVCRT__iob,0,3*sizeof(MSVCRT_FILE));
  MSVCRT_handles[0] = GetStdHandle(STD_INPUT_HANDLE);
  MSVCRT_flags[0] = MSVCRT__iob[0]._flag = MSVCRT__IOREAD;
  MSVCRT_handles[1] = GetStdHandle(STD_OUTPUT_HANDLE);
  MSVCRT_flags[1] = MSVCRT__iob[1]._flag = MSVCRT__IOWRT;
  MSVCRT_handles[2] = GetStdHandle(STD_ERROR_HANDLE);
  MSVCRT_flags[2] = MSVCRT__iob[2]._flag = MSVCRT__IOWRT;

  TRACE(":handles (%p)(%p)(%p)\n",MSVCRT_handles[0],
	MSVCRT_handles[1],MSVCRT_handles[2]);

  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    MSVCRT_files[i] = &MSVCRT__iob[i];
    MSVCRT__iob[i]._file = i;
    MSVCRT_tempfiles[i] = NULL;
  }
}

/* free everything on process exit */
void msvcrt_free_io(void)
{
    _fcloseall();
    _close(0);
    _close(1);
    _close(2);
}

/* INTERNAL: Flush stdio file buffer */
static int msvcrt_flush_buffer(MSVCRT_FILE* file)
{
  if(file->_bufsiz) {
        int cnt=file->_ptr-file->_base;
        if(cnt>0 && _write(file->_file, file->_base, cnt) != cnt) {
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
		file->_base = (unsigned char *)(&file->_charbuf);
		/* put here 2 ??? */
		file->_bufsiz = sizeof(file->_charbuf);
	}
	file->_ptr = file->_base;
	file->_cnt = 0;
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

  if (!filename || attr == 0xffffffff)
  {
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & MSVCRT_W_OK))
  {
    MSVCRT__set_errno(ERROR_ACCESS_DENIED);
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

  if (!filename || attr == 0xffffffff)
  {
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & MSVCRT_W_OK))
  {
    MSVCRT__set_errno(ERROR_ACCESS_DENIED);
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

  if (oldFlags != 0x0FFFFFFFF)
  {
    DWORD newFlags = (flags & _S_IWRITE)? oldFlags & ~FILE_ATTRIBUTE_READONLY:
      oldFlags | FILE_ATTRIBUTE_READONLY;

    if (newFlags == oldFlags || SetFileAttributesA(path, newFlags))
      return 0;
  }
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wchmod (MSVCRT.@)
 */
int _wchmod(const MSVCRT_wchar_t *path, int flags)
{
  DWORD oldFlags = GetFileAttributesW(path);

  if (oldFlags != 0x0FFFFFFFF)
  {
    DWORD newFlags = (flags & _S_IWRITE)? oldFlags & ~FILE_ATTRIBUTE_READONLY:
      oldFlags | FILE_ATTRIBUTE_READONLY;

    if (newFlags == oldFlags || SetFileAttributesW(path, newFlags))
      return 0;
  }
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_chsize (MSVCRT.@)
 */
int _chsize(int fd, long size)
{
    FIXME("(fd=%d, size=%ld): stub\n", fd, size);
    return -1;
}

/*********************************************************************
 *		_dup (MSVCRT.@)
 */
int _dup(int od)
{
    FIXME("(od=%d): stub\n", od);
    return -1;
}

/*********************************************************************
 *		_dup2 (MSVCRT.@)
 */
int _dup2(int od, int nd)
{
    FIXME("(od=%d, nd=%d): stub\n", od, nd);
    return -1;
}

/*********************************************************************
 *		_unlink (MSVCRT.@)
 */
int _unlink(const char *path)
{
  TRACE("(%s)\n",path);
  if(DeleteFileA(path))
    return 0;
  TRACE("failed (%ld)\n",GetLastError());
  MSVCRT__set_errno(GetLastError());
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
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_close (MSVCRT.@)
 */
int _close(int fd)
{
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;
  /* flush stdio buffers */
  if(MSVCRT_files[fd]) {
  	if(MSVCRT_files[fd]->_flag & MSVCRT__IOWRT)
		MSVCRT_fflush(MSVCRT_files[fd]);

  	if(MSVCRT_files[fd]->_flag & MSVCRT__IOMYBUF)
		MSVCRT_free(MSVCRT_files[fd]->_base);
  }

  /* Dont free std FILE*'s, they are not dynamic */
  if (fd > 2 && MSVCRT_files[fd])
    MSVCRT_free(MSVCRT_files[fd]);

  msvcrt_free_fd(fd);

  if (!CloseHandle(hand))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  if (MSVCRT_tempfiles[fd])
  {
    TRACE("deleting temporary file '%s'\n",MSVCRT_tempfiles[fd]);
    _unlink(MSVCRT_tempfiles[fd]);
    MSVCRT_free(MSVCRT_tempfiles[fd]);
    MSVCRT_tempfiles[fd] = NULL;
  }

  TRACE(":ok\n");
  return 0;
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
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  TRACE(":ok\n");
  return 0;
}

/*********************************************************************
 *		_eof (MSVCRT.@)
 */
int _eof(int fd)
{
  DWORD curpos,endpos;
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* If we have a FILE* for this file, the EOF flag
   * will be set by the read()/write() functions.
   */
  if (MSVCRT_files[fd])
    return MSVCRT_flags[fd] & MSVCRT__IOEOF;

  /* Otherwise we do it the hard way */
  curpos = SetFilePointer(hand, 0, NULL, SEEK_CUR);
  endpos = SetFilePointer(hand, 0, NULL, FILE_END);

  if (curpos == endpos)
    return TRUE;

  SetFilePointer(hand, curpos, 0, FILE_BEGIN);
  return FALSE;
}

/*********************************************************************
 *		_fcloseall (MSVCRT.@)
 */
int _fcloseall(void)
{
  int num_closed = 0, i;

  for (i = 3; i < MSVCRT_fdend; i++)
    if (MSVCRT_handles[i] != INVALID_HANDLE_VALUE)
    {
      _close(i);
      num_closed++;
    }

  TRACE(":closed (%d) handles\n",num_closed);
  return num_closed;
}

/*********************************************************************
 *		_lseek (MSVCRT.@)
 */
__int64 _lseeki64(int fd, __int64 offset, int whence)
{
  DWORD ret, hoffset = (DWORD) (offset >> 32);
  HANDLE hand = msvcrt_fdtoh(fd);

  TRACE(":fd (%d) handle (%p)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (whence < 0 || whence > 2)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  TRACE(":fd (%d) to 0x%08lx%08lx pos %s\n",
        fd,hoffset,(long)offset,
        (whence==SEEK_SET)?"SEEK_SET":
        (whence==SEEK_CUR)?"SEEK_CUR":
        (whence==SEEK_END)?"SEEK_END":"UNKNOWN");

  if (((ret = SetFilePointer(hand, (long)offset, &hoffset,
                             whence)) != INVALID_SET_FILE_POINTER) || !GetLastError())
  {
    if (MSVCRT_files[fd])
      MSVCRT_files[fd]->_flag &= ~MSVCRT__IOEOF;
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    return ((__int64)hoffset << 32) | ret;
  }
  TRACE(":error-last error (%ld)\n",GetLastError());
  if (MSVCRT_files[fd])
    switch(GetLastError())
    {
    case ERROR_NEGATIVE_SEEK:
    case ERROR_SEEK_ON_DEVICE:
      MSVCRT__set_errno(GetLastError());
      MSVCRT_files[fd]->_flag |= MSVCRT__IOERR;
      break;
    default:
      break;
    }
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
        fd,nbytes,(mode==_LK_UNLCK)?"_LK_UNLCK":
        (mode==_LK_LOCK)?"_LK_LOCK":
        (mode==_LK_NBLCK)?"_LK_NBLCK":
        (mode==_LK_RLCK)?"_LK_RLCK":
        (mode==_LK_NBRLCK)?"_LK_NBRLCK":
                          "UNKNOWN");

  if ((cur_locn = SetFilePointer(hand, 0L, NULL, SEEK_CUR)) == INVALID_SET_FILE_POINTER)
  {
    FIXME ("Seek failed\n");
    *MSVCRT__errno() = MSVCRT_EINVAL; /* FIXME */
    return -1;
  }
  if (mode == _LK_LOCK || mode == _LK_RLCK)
  {
    int nretry = 10;
    ret = 1; /* just to satisfy gcc */
    while (nretry--)
    {
      ret = LockFile(hand, cur_locn, 0L, nbytes, 0L);
      if (ret) break;
      sleep (1);
    }
  }
  else if (mode == _LK_UNLCK)
    ret = UnlockFile(hand, cur_locn, 0L, nbytes, 0L);
  else
    ret = LockFile(hand, cur_locn, 0L, nbytes, 0L);
  /* FIXME - what about error settings? */
  return ret ? 0 : -1;
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

/*********************************************************************
 *		_fdopen (MSVCRT.@)
 */
MSVCRT_FILE* _fdopen(int fd, const char *mode)
{
  MSVCRT_FILE* file = msvcrt_alloc_fp(fd);

  TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,mode,file);

  return file;
}

/*********************************************************************
 *		_wfdopen (MSVCRT.@)
 */
MSVCRT_FILE* _wfdopen(int fd, const MSVCRT_wchar_t *mode)
{
  MSVCRT_FILE* file = msvcrt_alloc_fp(fd);

  TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,debugstr_w(mode),file);
  if (file)
    MSVCRT_rewind(file);

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
 *		_fileno (MSVCRT.@)
 */
int _fileno(MSVCRT_FILE* file)
{
  TRACE(":FILE* (%p) fd (%d)\n",file,file->_file);
  return file->_file;
}

/*********************************************************************
 *		_flushall (MSVCRT.@)
 */
int _flushall(void)
{
  int num_flushed = 0, i = 3;

  while(i < MSVCRT_fdend)
    if (MSVCRT_handles[i] != INVALID_HANDLE_VALUE)
    {
#if 0
      /* FIXME: flush, do not commit */
      if (_commit(i) == -1)
	if (MSVCRT_files[i])
	  MSVCRT_files[i]->_flag |= MSVCRT__IOERR;
#endif
      if(MSVCRT_files[i] && MSVCRT_files[i]->_flag & MSVCRT__IOWRT) {
	MSVCRT_fflush(MSVCRT_files[i]);
        num_flushed++;
      }
    }

  TRACE(":flushed (%d) handles\n",num_flushed);
  return num_flushed;
}

/*********************************************************************
 *		_fstati64 (MSVCRT.@)
 */
int _fstati64(int fd, struct _stati64* buf)
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
    MSVCRT__set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct _stati64));
  if (!GetFileInformationByHandle(hand, &hfi))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    MSVCRT__set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }
  FIXME(":dwFileAttributes = %ld, mode set to 0\n",hfi.dwFileAttributes);
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
int MSVCRT__fstat(int fd, struct _stat* buf)
{ int ret;
  struct _stati64 bufi64;

  ret = _fstati64(fd, &bufi64);
  if (!ret)
      msvcrt_cp_from_stati64(&bufi64, buf);
  return ret;
}

/*********************************************************************
 *		_futime (MSVCRT.@)
 */
int _futime(int fd, struct _utimbuf *t)
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
    MSVCRT__set_errno(GetLastError());
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
  HANDLE newhand = hand;
  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  if (hand != INVALID_HANDLE_VALUE)
  {
    /* FIXME: I'm not convinced that I should be copying the
     * handle here - it may be leaked if the app doesn't
     * close it (and the API docs dont say that it should)
     * Not duplicating it means that it can't be inherited
     * and so lcc's wedit doesn't cope when it passes it to
     * child processes. I've an idea that it should either
     * be copied by CreateProcess, or marked as inheritable
     * when initialised, or maybe both? JG 21-9-00.
     */
    DuplicateHandle(GetCurrentProcess(),hand,GetCurrentProcess(),
		    &newhand,0,TRUE,DUPLICATE_SAME_ACCESS);
  }
  return (long)newhand;
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
    if (GetFileAttributesA(retVal) == 0xFFFFFFFF &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      return retVal;
    *pattern = letter++;
  } while(letter != '|');
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
    if (GetFileAttributesW(retVal) == 0xFFFFFFFF &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
      return retVal;
    *pattern = letter++;
  } while(letter != '|');
  return NULL;
}

/*********************************************************************
 *              _sopen (MSVCRT.@)
 */
int MSVCRT__sopen( const char *path, int oflags, int shflags, ... )
{
  va_list ap;
  int pmode;
  DWORD access = 0, creation = 0;
  DWORD sharing;
  int ioflag = 0, fd;
  HANDLE hand;
  SECURITY_ATTRIBUTES sa;


  TRACE(":file (%s) oflags: 0x%04x shflags: 0x%04x\n",
        path, oflags, shflags);

  switch(oflags & (_O_RDONLY | _O_WRONLY | _O_RDWR))
  {
  case _O_RDONLY:
    access |= GENERIC_READ;
    ioflag |= MSVCRT__IOREAD;
    break;
  case _O_WRONLY:
    access |= GENERIC_WRITE;
    ioflag |= MSVCRT__IOWRT;
    break;
  case _O_RDWR:
    access |= GENERIC_WRITE | GENERIC_READ;
    ioflag |= MSVCRT__IORW;
    break;
  }

  if (oflags & _O_CREAT)
  {
    va_start(ap, shflags);
      pmode = va_arg(ap, int);
    va_end(ap);

    if(pmode & ~(_S_IREAD | _S_IWRITE))
      FIXME(": pmode 0x%04x ignored\n", pmode);
    else
      WARN(": pmode 0x%04x ignored\n", pmode);

    if (oflags & _O_EXCL)
      creation = CREATE_NEW;
    else if (oflags & _O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  }
  else  /* no _O_CREAT */
  {
    if (oflags & _O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }
  if (oflags & _O_APPEND)
    ioflag |= MSVCRT__IOAPPEND;

  if (oflags & _O_BINARY)
    ioflag |= _O_BINARY;
  else if (oflags & _O_TEXT)
    ioflag |= _O_TEXT;
  else if (*__p__fmode() & _O_BINARY)
    ioflag |= _O_BINARY;
  else
    ioflag |= _O_TEXT; /* default to TEXT*/
  
  switch( shflags )
  {
    case _SH_DENYRW:
      sharing = 0L;
      break;
    case _SH_DENYWR:
      sharing = FILE_SHARE_READ;
      break;
    case _SH_DENYRD:
      sharing = FILE_SHARE_WRITE;
      break;
    case _SH_DENYNO:
      sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
    default:
      ERR( "Unhandled shflags 0x%x\n", shflags );
      return -1;
  }

  if (oflags & ~(_O_BINARY|_O_TEXT|_O_APPEND|_O_TRUNC|_O_EXCL
                |_O_CREAT|_O_RDWR|_O_TEMPORARY|_O_NOINHERIT))
    TRACE(":unsupported oflags 0x%04x\n",oflags);

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = (oflags & _O_NOINHERIT) ? FALSE : TRUE;

  hand = CreateFileA(path, access, sharing,
                      &sa, creation, FILE_ATTRIBUTE_NORMAL, 0);

  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    MSVCRT__set_errno(GetLastError());
    return -1;
  }

  fd = msvcrt_alloc_fd(hand, ioflag);

  TRACE(":fd (%d) handle (%p)\n",fd, hand);

  if (fd > 0)
  {
    if (oflags & _O_TEMPORARY)
      MSVCRT_tempfiles[fd] = _strdup(path);
    if (ioflag & MSVCRT__IOAPPEND)
      _lseek(fd, 0, FILE_END);
  }

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

  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *              _open (MSVCRT.@)
 */
int _open( const char *path, int flags, ... )
{
  va_list ap;

  if (flags & _O_CREAT)
  {
    int pmode;
    va_start(ap, flags);
    pmode = va_arg(ap, int);
    va_end(ap);
    return MSVCRT__sopen( path, flags, _SH_DENYNO, pmode );
  }
  else
    return MSVCRT__sopen( path, flags, _SH_DENYNO);
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

  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_creat (MSVCRT.@)
 */
int _creat(const char *path, int flags)
{
  int usedFlags = (flags & _O_TEXT)| _O_CREAT| _O_WRONLY| _O_TRUNC;
  return _open(path, usedFlags);
}

/*********************************************************************
 *		_wcreat (MSVCRT.@)
 */
int _wcreat(const MSVCRT_wchar_t *path, int flags)
{
  int usedFlags = (flags & _O_TEXT)| _O_CREAT| _O_WRONLY| _O_TRUNC;
  return _wopen(path, usedFlags);
}

/*********************************************************************
 *		_open_osfhandle (MSVCRT.@)
 */
int _open_osfhandle(long hand, int flags)
{
  int fd;

  /* _O_RDONLY (0) always matches, so set the read flag
   * MFC's CStdioFile clears O_RDONLY (0)! if it wants to write to the
   * file, so set the write flag. It also only sets _O_TEXT if it wants
   * text - it never sets _O_BINARY.
   */
  /* FIXME: handle more flags */
  flags |= MSVCRT__IOREAD|MSVCRT__IOWRT;
  if ( !( flags & _O_TEXT ) ) flags |= _O_BINARY;

  fd = msvcrt_alloc_fd((HANDLE)hand,flags);
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n",hand,fd, flags);
  return fd;
}

/*********************************************************************
 *		_rmtmp (MSVCRT.@)
 */
int _rmtmp(void)
{
  int num_removed = 0, i;

  for (i = 3; i < MSVCRT_fdend; i++)
    if (MSVCRT_tempfiles[i])
    {
      _close(i);
      num_removed++;
    }

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
  DWORD num_read, all_read =0;
  char *bufstart = buf;
  HANDLE hand = msvcrt_fdtoh(fd);

  /* Dont trace small reads, it gets *very* annoying */
  if (count > 4)
    TRACE(":fd (%d) handle (%p) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Reading single bytes in O_TEXT mode makes things slow
   * So read big chunks, then remove the \r in memory and try reading
   * the rest until the request is satisfied or EOF is met
   */
  while (  all_read < count)
  {
      if (ReadFile(hand, bufstart+all_read, count - all_read, &num_read, NULL))
      {
          if (num_read != (count- all_read))
          {
              TRACE(":EOF\n");
              if ( MSVCRT_files[fd])
              {
                  MSVCRT_flags[fd] |= MSVCRT__IOEOF;
                  /*
                    MSVCRT_files[fd]->_flag |= MSVCRT__IOEOF;
                  */
              }
              if ((MSVCRT_flags[fd]& _O_BINARY ) !=  _O_BINARY )
                  num_read -= remove_cr(bufstart+all_read,num_read);
              all_read += num_read;
              if (count > 4)
                  TRACE("%s\n",debugstr_an(buf,all_read));
              return all_read;
          }
          if ((MSVCRT_flags[fd]& _O_BINARY ) !=  _O_BINARY )
          {
              num_read -= remove_cr(bufstart+all_read,num_read);
          }
          all_read += num_read;
      }
      else
      {
          TRACE(":failed-last error (%ld)\n",GetLastError());
          if (MSVCRT_files[fd])
              MSVCRT_files[fd]->_flag |= MSVCRT__IOERR;
          return -1;
      }
  }

  if (count > 4)
      TRACE("%s\n",debugstr_an(buf, all_read));
  return all_read;
}

/*********************************************************************
 *		_getw (MSVCRT.@)
 */
int _getw(MSVCRT_FILE* file)
{
  int i;
  if (_read(file->_file, &i, sizeof(int)) != 1)
    return MSVCRT_EOF;
  return i;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int _setmode(int fd,int mode)
{
  int ret = MSVCRT_flags[fd] & (_O_TEXT | _O_BINARY);
  if (mode & (~(_O_TEXT|_O_BINARY)))
    FIXME("fd (%d) mode (0x%08x) unknown\n",fd,mode);
  MSVCRT_flags[fd] &= ~(_O_TEXT|_O_BINARY);
  MSVCRT_flags[fd] |= mode & (_O_TEXT | _O_BINARY);
  return ret;
}

/*********************************************************************
 *		_stati64 (MSVCRT.@)
 */
int _stati64(const char* path, struct _stati64 * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = MSVCRT_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%ld)\n",GetLastError());
      MSVCRT__set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct _stati64));

  /* FIXME: rdev isnt drive num,despite what the docs say-what is it?
     Bon 011120: This FIXME seems incorrect
                 Also a letter as first char isn't enough to be classify
		 as drive letter
  */
  if (isalpha(*path)&& (*(path+1)==':'))
    buf->st_dev = buf->st_rdev = toupper(*path) - 'A'; /* drive num */
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

  plen = strlen(path);

  /* Dir, or regular file? */
  if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
      (path[plen-1] == '\\'))
    mode |= (_S_IFDIR | MSVCRT_S_IEXEC);
  else
  {
    mode |= _S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
      unsigned int ext = tolower(path[plen-1]) | (tolower(path[plen-2]) << 8) |
                                 (tolower(path[plen-3]) << 16);
      if (ext == EXE || ext == BAT || ext == CMD || ext == COM)
          mode |= MSVCRT_S_IEXEC;
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= MSVCRT_S_IWRITE;

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
int MSVCRT__stat(const char* path, struct _stat * buf)
{ int ret;
  struct _stati64 bufi64;

  ret = _stati64( path, &bufi64);
  if (!ret)
      msvcrt_cp_from_stati64(&bufi64, buf);
  return ret;
}

/*********************************************************************
 *		_wstat (MSVCRT.@)
 */
int _wstat(const MSVCRT_wchar_t* path, struct _stat * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = MSVCRT_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",debugstr_w(path),buf);

  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%ld)\n",GetLastError());
      MSVCRT__set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct _stat));

  /* FIXME: rdev isn't drive num, despite what the docs says-what is it? */
  if (MSVCRT_iswalpha(*path))
    buf->st_dev = buf->st_rdev = toupperW(*path - 'A'); /* drive num */
  else
    buf->st_dev = buf->st_rdev = _getdrive() - 1;

  plen = strlenW(path);

  /* Dir, or regular file? */
  if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
      (path[plen-1] == '\\'))
    mode |= (_S_IFDIR | MSVCRT_S_IEXEC);
  else
  {
    mode |= _S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
      ULONGLONG ext = tolowerW(path[plen-1]) | (tolowerW(path[plen-2]) << 16) |
                               ((ULONGLONG)tolowerW(path[plen-3]) << 32);
      if (ext == WCEXE || ext == WCBAT || ext == WCCMD || ext == WCCOM)
        mode |= MSVCRT_S_IEXEC;
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= MSVCRT_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
  buf->st_size  = hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970((LARGE_INTEGER *)&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("\n%d %d %d %ld %ld %ld\n", buf->st_mode,buf->st_nlink,buf->st_size,
        buf->st_atime,buf->st_mtime, buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_tell (MSVCRT.@)
 */
LONG _tell(int fd)
{
  return _lseek(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_tempnam (MSVCRT.@)
 */
char *_tempnam(const char *dir, const char *prefix)
{
  char tmpbuf[MAX_PATH];

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
int _utime(const char* path, struct _utimbuf *t)
{
  int fd = _open(path, _O_WRONLY | _O_BINARY);

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
int _wutime(const MSVCRT_wchar_t* path, struct _utimbuf *t)
{
  int fd = _wopen(path, _O_WRONLY | _O_BINARY);

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

  /* Dont trace small writes, it gets *very* annoying */
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
  if (MSVCRT_flags[fd] & MSVCRT__IOAPPEND)
    _lseek(fd, 0, FILE_END);

  if (MSVCRT_flags[fd] & _O_BINARY)
    {
      if (WriteFile(hand, buf, count, &num_written, NULL)
	  &&  (num_written == count))
	return num_written;
      TRACE(":failed-last error (%ld)\n",GetLastError());
      if (MSVCRT_files[fd])
	{
	  MSVCRT_files[fd]->_flag |= MSVCRT__IOERR;
	  *MSVCRT__errno() = MSVCRT_ENOSPC;
	}
    }
  else
  {
      unsigned int i, j, nr_lf;
      char *s=(char*)buf, *buf_start=(char*)buf, *p;
      /* find number of \n ( without preceeding \r */
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
          TRACE(":failed-last error (%ld) num_written %ld\n",GetLastError(),num_written);
          if (MSVCRT_files[fd])
          {
              MSVCRT_files[fd]->_flag |= MSVCRT__IOERR;
              *MSVCRT__errno() = MSVCRT_ENOSPC;
              if(nr_lf)
                  MSVCRT_free(p);
              return s - buf_start;
          }
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
int _putw(int val, MSVCRT_FILE* file)
{
  return _write(file->_file, &val, sizeof(val)) == 1? val : MSVCRT_EOF;
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
 *		fclose (MSVCRT.@)
 */
int MSVCRT_fclose(MSVCRT_FILE* file)
{
  int r, flag;

  flag = file->_flag;
  r=_close(file->_file);
  return ((r==MSVCRT_EOF) || (flag & MSVCRT__IOERR) ? MSVCRT_EOF : 0);
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
 *		fflush (MSVCRT.@)
 */
int MSVCRT_fflush(MSVCRT_FILE* file)
{
  if(!file) {
	_flushall();
	return 0;
  } else {
  	int res=msvcrt_flush_buffer(file);
  	return res;
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
	return _filbuf(file);
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
 *		_filbuf (MSVCRT.@)
 */
int _filbuf(MSVCRT_FILE* file)
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
  	if (_read(file->_file,&c,1) != 1) {
		file->_flag |= MSVCRT__IOEOF;
    		return MSVCRT_EOF;
	}
  	return c;
  } else {
	file->_cnt = _read(file->_file, file->_base, file->_bufsiz);
	if(file->_cnt<0) file->_cnt = 0;
	if(!file->_cnt) {
		file->_flag |= MSVCRT__IOEOF;
		return MSVCRT_EOF;
	}
	file->_cnt--;
	file->_ptr = file->_base+1;
	return *(unsigned char *)file->_base;
  }
}

/*********************************************************************
 *		fgetpos (MSVCRT.@)
 */
int MSVCRT_fgetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
  *pos = MSVCRT_ftell(file);
  return (*pos == -1? -1 : 0);
}

/*********************************************************************
 *		fgets (MSVCRT.@)
 */
char *MSVCRT_fgets(char *s, int size, MSVCRT_FILE* file)
{
  int    cc;
  char * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
	file,file->_file,s,size);

  for(cc = MSVCRT_fgetc(file); cc != MSVCRT_EOF && cc != '\n';
      cc = MSVCRT_fgetc(file))
    /* _read already handled the translation */
    {
      if (--size <= 0) break;
      *s++ = (char)cc;
    }
  if ((cc == MSVCRT_EOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    return 0;
  }
  if (cc == '\n')
    if (--size > 0)
      *s++ = '\n';
  *s = '\0';
  TRACE(":got '%s'\n", debugstr_a(buf_start));
  return buf_start;
}

/*********************************************************************
 *		fgetwc (MSVCRT.@)
 *
 * In _O_TEXT mode, bultibyte characters are read from the file, dropping
 * the CR from CR/LF combinations
 */
MSVCRT_wint_t MSVCRT_fgetwc(MSVCRT_FILE* file)
{
  char c;

  if (file->_flag & _O_BINARY)
    {
      MSVCRT_wchar_t wc;
      if (_read(file->_file, &wc, sizeof(wc)) != sizeof(wc))
	return MSVCRT_WEOF;
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
  int    cc;
  MSVCRT_wchar_t * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
        file,file->_file,s,size);

  for(cc = MSVCRT_fgetwc(file); cc != MSVCRT_WEOF && cc != L'\n';
      cc = MSVCRT_fgetwc(file))
    /* _read already handled the translation */
    {
      if (--size <= 0) break;
      *s++ = cc;
    }
  if ((cc == MSVCRT_EOF) && (s == buf_start)) /* If nothing read, return 0*/
  {
    TRACE(":nothing read\n");
    return 0;
  }
  if (cc == L'\n')
    if (--size > 0)
      *s++ = '\n';
  *s = '\0';
/*  TRACE(":got '%s'\n", buf_start); */
  return buf_start;
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
  int flags = 0, plus = 0, fd;
  const char* search = mode;

  TRACE("(%s,%s)\n",path,mode);

  while (*search)
    if (*search++ == '+')
      plus = 1;

  /* map mode string to open() flags. "man fopen" for possibilities. */
  switch(*mode++)
  {
  case 'R': case 'r':
    flags = (plus ? _O_RDWR : _O_RDONLY);
    break;
  case 'W': case 'w':
    flags = _O_CREAT | _O_TRUNC | (plus  ? _O_RDWR : _O_WRONLY);
    break;
  case 'A': case 'a':
    flags = _O_CREAT | _O_APPEND | (plus  ? _O_RDWR : _O_WRONLY);
    break;
  default:
    return NULL;
  }

  while (*mode)
    switch (*mode++)
    {
    case 'B': case 'b':
      flags |=  _O_BINARY;
      flags &= ~_O_TEXT;
      break;
    case 'T': case 't':
      flags |=  _O_TEXT;
      flags &= ~_O_BINARY;
      break;
    case '+':
      break;
    default:
      FIXME(":unknown flag %c not supported\n",mode[-1]);
    }

  fd = _open(path, flags);

  if (fd < 0)
    return NULL;

  file = msvcrt_alloc_fp(fd);
  TRACE(":got (%p)\n",file);
  if (!file)
    _close(fd);

  return file;
}

/*********************************************************************
 *		_wfopen (MSVCRT.@)
 */
MSVCRT_FILE *_wfopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode)
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

  MSVCRT__set_errno(GetLastError());
  return NULL;
}

/*********************************************************************
 *		_fsopen (MSVCRT.@)
 */
MSVCRT_FILE*  _fsopen(const char *path, const char *mode, int share)
{
  FIXME(":(%s,%s,%d),ignoring share mode!\n",path,mode,share);
  return MSVCRT_fopen(path,mode);
}

/*********************************************************************
 *		_wfsopen (MSVCRT.@)
 */
MSVCRT_FILE*  _wfsopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode, int share)
{
  FIXME(":(%s,%s,%d),ignoring share mode!\n",
        debugstr_w(path),debugstr_w(mode),share);
  return _wfopen(path,mode);
}

/*********************************************************************
 *		fputc (MSVCRT.@)
 */
int MSVCRT_fputc(int c, MSVCRT_FILE* file)
{
  if(file->_cnt>0) {
	*file->_ptr++=c;
	file->_cnt--;
	return c;
  } else {
	return _flsbuf(c, file);
  }
}

/*********************************************************************
 *		_flsbuf (MSVCRT.@)
 */
int _flsbuf(int c, MSVCRT_FILE* file)
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
  	return _write(file->_file, &cc, 1) == 1? c : MSVCRT_EOF;
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
  if(rcnt) pread = _read(file->_file,ptr, rcnt);
  if (MSVCRT_flags[file->_file] & MSVCRT__IOEOF)
     /* expose feof condition in the flags
        MFC tests file->_flag for feof, and doesn't not call feof())
    */
    file->_flag |= MSVCRT__IOEOF;
  if (pread <= 0)
    pread = 0;
  read+=pread;
  return read / size;
}

/*********************************************************************
 *		freopen (MSVCRT.@)
 *
 */
MSVCRT_FILE* MSVCRT_freopen(const char *path, const char *mode,MSVCRT_FILE* file)
{
  MSVCRT_FILE* newfile;
  int fd;

  TRACE(":path (%p) mode (%s) file (%p) fd (%d)\n",path,mode,file,file->_file);
  if (!file || ((fd = file->_file) < 0) || fd > MSVCRT_fdend)
    return NULL;

  if (fd > 2)
  {
#if 0
    FIXME(":reopen on user file not implemented!\n");
    MSVCRT__set_errno(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
#endif
    if(MSVCRT_fclose(file))
	return NULL;
    return MSVCRT_fopen(path, mode);
  }

  /* first, create the new file */
  if ((newfile = MSVCRT_fopen(path,mode)) == NULL)
    return NULL;

  if (fd < 3 && SetStdHandle(fd == 0 ? STD_INPUT_HANDLE :
     (fd == 1? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE),
      MSVCRT_handles[newfile->_file]))
  {
    /* Redirecting std handle to file , copy over.. */
    MSVCRT_handles[fd] = MSVCRT_handles[newfile->_file];
    MSVCRT_flags[fd] = MSVCRT_flags[newfile->_file];
    memcpy(&MSVCRT__iob[fd], newfile, sizeof (MSVCRT_FILE));
    MSVCRT__iob[fd]._file = fd;
    /* And free up the resources allocated by fopen, but
     * not the HANDLE we copied. */
    MSVCRT_free(MSVCRT_files[fd]);
    msvcrt_free_fd(newfile->_file);
    return &MSVCRT__iob[fd];
  }

  WARN(":failed-last error (%ld)\n",GetLastError());
  MSVCRT_fclose(newfile);
  MSVCRT__set_errno(GetLastError());
  return NULL;
}

/*********************************************************************
 *		fsetpos (MSVCRT.@)
 */
int MSVCRT_fsetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
  return _lseek(file->_file,*pos,SEEK_SET);
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
       ptr = (char*)ptr + pcnt;
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
  		if (pwritten <= 0) pwritten=0;
		written += pwritten;
	}
  }
  return written / size;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int MSVCRT_fputs(const char *s, MSVCRT_FILE* file)
{
    size_t i, len = strlen(s);
    if (file->_flag & _O_BINARY)
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
    if (file->_flag & _O_BINARY)
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
  MSVCRT__set_errno(GetLastError());
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
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		scanf (MSVCRT.@)
 */
int MSVCRT_scanf(const char *format, ...)
{
  va_list valist;
  int res;

  va_start(valist, format);
  res = MSVCRT_fscanf(MSVCRT_stdin, format, valist);
  va_end(valist);
  return res;
}

/*********************************************************************
 *		wscanf (MSVCRT.@)
 */
int MSVCRT_wscanf(const MSVCRT_wchar_t *format, ...)
{
  va_list valist;
  int res;

  va_start(valist, format);
  res = MSVCRT_fwscanf(MSVCRT_stdin, format, valist);
  va_end(valist);
  return res;
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
  MSVCRT__set_errno(GetLastError());
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
  MSVCRT__set_errno(GetLastError());
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
  char tmpbuf[MAX_PATH];
  char* prefix = "TMP";
  if (!GetTempPathA(MAX_PATH,tmpbuf) ||
      !GetTempFileNameA(tmpbuf,prefix,0,MSVCRT_tmpname))
  {
    TRACE(":failed-last error (%ld)\n",GetLastError());
    return NULL;
  }
  TRACE(":got tmpnam %s\n",MSVCRT_tmpname);
  s = MSVCRT_tmpname;
  return s;
}

/*********************************************************************
 *		tmpfile (MSVCRT.@)
 */
MSVCRT_FILE* MSVCRT_tmpfile(void)
{
  char *filename = MSVCRT_tmpnam(NULL);
  int fd;
  fd = _open(filename, _O_CREAT | _O_BINARY | _O_RDWR | _O_TEMPORARY);
  if (fd != -1)
    return msvcrt_alloc_fp(fd);
  return NULL;
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
  while ((written = vsnprintf(mem, resize, format, valist)) == -1 ||
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
  while ((written = _vsnwprintf(mem, resize, format, valist)) == -1 ||
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
	if(file->_bufsiz == 0 && !(file->_flag & MSVCRT__IONBF)) {
		msvcrt_alloc_buffer(file);
		file->_ptr++;
	}
	if(file->_ptr>file->_base) {
		file->_ptr--;
		*file->_ptr=c;
		file->_cnt++;
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
