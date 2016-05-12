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

#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <limits.h>

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
#define WX_READNL         0x04  /* read started with \n */
#define WX_PIPE           0x08
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TTY            0x40
#define WX_TEXT           0x80

/* values for exflag - it's used differently in msvcr90.dll*/
#define EF_UTF8           0x01
#define EF_UTF16          0x02
#define EF_CRIT_INIT      0x04
#define EF_UNK_UNICODE    0x08

static char utf8_bom[3] = { 0xef, 0xbb, 0xbf };
static char utf16_bom[2] = { 0xff, 0xfe };

/* FIXME: this should be allocated dynamically */
#define MSVCRT_MAX_FILES 2048
#define MSVCRT_FD_BLOCK_SIZE 32

#define MSVCRT_INTERNAL_BUFSIZ 4096

/* ioinfo structure size is different in msvcrXX.dll's */
typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    char                lookahead[3];
    int                 exflag;
    CRITICAL_SECTION    crit;
#if _MSVCR_VER >= 80
    char textmode : 7;
    char unicode : 1;
    char pipech2[2];
    __int64 startpos;
    BOOL utf8translations;
#endif
#if _MSVCR_VER >= 90
    char dbcsBuffer;
    BOOL dbcsBufferUsed;
#endif
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
static int tmpnam_s_unique;

static const unsigned int EXE = 'e' << 16 | 'x' << 8 | 'e';
static const unsigned int BAT = 'b' << 16 | 'a' << 8 | 't';
static const unsigned int CMD = 'c' << 16 | 'm' << 8 | 'd';
static const unsigned int COM = 'c' << 16 | 'o' << 8 | 'm';

#define TOUL(x) (ULONGLONG)(x)
static const ULONGLONG WCEXE = TOUL('e') << 32 | TOUL('x') << 16 | TOUL('e');
static const ULONGLONG WCBAT = TOUL('b') << 32 | TOUL('a') << 16 | TOUL('t');
static const ULONGLONG WCCMD = TOUL('c') << 32 | TOUL('m') << 16 | TOUL('d');
static const ULONGLONG WCCOM = TOUL('c') << 32 | TOUL('o') << 16 | TOUL('m');

/* This critical section protects the MSVCRT_fstreams table
 * and MSVCRT_stream_idx from race conditions. It also
 * protects fd critical sections creation code.
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

static void msvcrt_stat64_to_stat32(const struct MSVCRT__stat64 *buf64, struct MSVCRT__stat32 *buf)
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

static void msvcrt_stat64_to_stat64i32(const struct MSVCRT__stat64 *buf64, struct MSVCRT__stat64i32 *buf)
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

static void msvcrt_stat64_to_stat32i64(const struct MSVCRT__stat64 *buf64, struct MSVCRT__stat32i64 *buf)
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

static inline ioinfo* get_ioinfo_nolock(int fd)
{
    ioinfo *ret = NULL;
    if(fd>=0 && fd<MSVCRT_MAX_FILES)
        ret = MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE];
    if(!ret)
        return &MSVCRT___badioinfo;

    return ret + (fd%MSVCRT_FD_BLOCK_SIZE);
}

static inline void init_ioinfo_cs(ioinfo *info)
{
    if(!(info->exflag & EF_CRIT_INIT)) {
        LOCK_FILES();
        if(!(info->exflag & EF_CRIT_INIT)) {
            InitializeCriticalSection(&info->crit);
            info->exflag |= EF_CRIT_INIT;
        }
        UNLOCK_FILES();
    }
}

static inline ioinfo* get_ioinfo(int fd)
{
    ioinfo *ret = get_ioinfo_nolock(fd);
    if(ret == &MSVCRT___badioinfo)
        return ret;
    init_ioinfo_cs(ret);
    EnterCriticalSection(&ret->crit);
    return ret;
}

static inline BOOL alloc_pioinfo_block(int fd)
{
    ioinfo *block;
    int i;

    if(fd<0 || fd>=MSVCRT_MAX_FILES)
    {
        *MSVCRT__errno() = MSVCRT_ENFILE;
        return FALSE;
    }

    block = MSVCRT_calloc(MSVCRT_FD_BLOCK_SIZE, sizeof(ioinfo));
    if(!block)
    {
        WARN(":out of memory!\n");
        *MSVCRT__errno() = MSVCRT_ENOMEM;
        return FALSE;
    }
    for(i=0; i<MSVCRT_FD_BLOCK_SIZE; i++)
        block[i].handle = INVALID_HANDLE_VALUE;
    if(InterlockedCompareExchangePointer((void**)&MSVCRT___pioinfo[fd/MSVCRT_FD_BLOCK_SIZE], block, NULL))
        MSVCRT_free(block);
    return TRUE;
}

static inline ioinfo* get_ioinfo_alloc_fd(int fd)
{
    ioinfo *ret;

    ret = get_ioinfo(fd);
    if(ret != &MSVCRT___badioinfo)
        return ret;

    if(!alloc_pioinfo_block(fd))
        return &MSVCRT___badioinfo;

    return get_ioinfo(fd);
}

static inline ioinfo* get_ioinfo_alloc(int *fd)
{
    int i;

    *fd = -1;
    for(i=0; i<MSVCRT_MAX_FILES; i++)
    {
        ioinfo *info = get_ioinfo_nolock(i);

        if(info == &MSVCRT___badioinfo)
        {
            if(!alloc_pioinfo_block(i))
                return &MSVCRT___badioinfo;
            info = get_ioinfo_nolock(i);
        }

        init_ioinfo_cs(info);
        if(TryEnterCriticalSection(&info->crit))
        {
            if(info->handle == INVALID_HANDLE_VALUE)
            {
                *fd = i;
                return info;
            }
            LeaveCriticalSection(&info->crit);
        }
    }

    WARN(":files exhausted!\n");
    *MSVCRT__errno() = MSVCRT_ENFILE;
    return &MSVCRT___badioinfo;
}

static inline void release_ioinfo(ioinfo *info)
{
    if(info!=&MSVCRT___badioinfo && info->exflag & EF_CRIT_INIT)
        LeaveCriticalSection(&info->crit);
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

/* INTERNAL: free a file entry fd */
static void msvcrt_free_fd(int fd)
{
  ioinfo *fdinfo = get_ioinfo(fd);

  if(fdinfo != &MSVCRT___badioinfo)
  {
    fdinfo->handle = INVALID_HANDLE_VALUE;
    fdinfo->wxflag = 0;
  }
  TRACE(":fd (%d) freed\n",fd);

  if (fd < 3)
  {
    switch (fd)
    {
    case 0:
        SetStdHandle(STD_INPUT_HANDLE, 0);
        break;
    case 1:
        SetStdHandle(STD_OUTPUT_HANDLE, 0);
        break;
    case 2:
        SetStdHandle(STD_ERROR_HANDLE, 0);
        break;
    }
  }
  release_ioinfo(fdinfo);
}

static void msvcrt_set_fd(ioinfo *fdinfo, HANDLE hand, int flag)
{
  fdinfo->handle = hand;
  fdinfo->wxflag = WX_OPEN | (flag & (WX_DONTINHERIT | WX_APPEND | WX_TEXT | WX_PIPE | WX_TTY));
  fdinfo->lookahead[0] = '\n';
  fdinfo->lookahead[1] = '\n';
  fdinfo->lookahead[2] = '\n';
  fdinfo->exflag &= EF_CRIT_INIT;

  switch (fdinfo-MSVCRT___pioinfo[0])
  {
  case 0: SetStdHandle(STD_INPUT_HANDLE,  hand); break;
  case 1: SetStdHandle(STD_OUTPUT_HANDLE, hand); break;
  case 2: SetStdHandle(STD_ERROR_HANDLE,  hand); break;
  }
}

/* INTERNAL: Allocate an fd slot from a Win32 HANDLE */
static int msvcrt_alloc_fd(HANDLE hand, int flag)
{
    int fd;
    ioinfo *info = get_ioinfo_alloc(&fd);

    TRACE(":handle (%p) allocating fd (%d)\n", hand, fd);

    if(info == &MSVCRT___badioinfo)
        return -1;

    msvcrt_set_fd(info, hand, flag);
    release_ioinfo(info);
    return fd;
}

/* INTERNAL: Allocate a FILE* for an fd slot */
/* caller must hold the files lock */
static MSVCRT_FILE* msvcrt_alloc_fp(void)
{
  int i;
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
  if (!(get_ioinfo_nolock(fd)->wxflag & WX_OPEN))
  {
    WARN(":invalid fd %d\n",fd);
    *MSVCRT___doserrno() = 0;
    *MSVCRT__errno() = MSVCRT_EBADF;
    return -1;
  }
  file->_ptr = file->_base = NULL;
  file->_cnt = 0;
  file->_file = fd;
  file->_flag = stream_flags;
  file->_tmpfname = NULL;

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
  int         fd, last_fd;
  char*       wxflag_ptr;
  HANDLE*     handle_ptr;
  ioinfo*     fdinfo;

  for (last_fd=MSVCRT_MAX_FILES-1; last_fd>=0; last_fd--)
    if (get_ioinfo_nolock(last_fd)->handle != INVALID_HANDLE_VALUE)
      break;
  last_fd++;

  *size = sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * last_fd;
  *block = MSVCRT_calloc(1, *size);
  if (!*block)
  {
    *size = 0;
    return FALSE;
  }
  wxflag_ptr = (char*)*block + sizeof(unsigned);
  handle_ptr = (HANDLE*)(wxflag_ptr + last_fd);

  *(unsigned*)*block = last_fd;
  for (fd = 0; fd < last_fd; fd++)
  {
    /* to be inherited, we need it to be open, and that DONTINHERIT isn't set */
    fdinfo = get_ioinfo(fd);
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
    release_ioinfo(fdinfo);
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
      {
        fdinfo = get_ioinfo_alloc_fd(i);
        if(fdinfo != &MSVCRT___badioinfo)
            msvcrt_set_fd(fdinfo, *handle_ptr, *wxflag_ptr);
        release_ioinfo(fdinfo);
      }

      wxflag_ptr++; handle_ptr++;
    }
  }

  fdinfo = get_ioinfo_alloc_fd(MSVCRT_STDIN_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD type = GetFileType(h);

    msvcrt_set_fd(fdinfo, h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_TTY : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0));
  }
  release_ioinfo(fdinfo);

  fdinfo = get_ioinfo_alloc_fd(MSVCRT_STDOUT_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD type = GetFileType(h);

    msvcrt_set_fd(fdinfo, h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_TTY : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0));
  }
  release_ioinfo(fdinfo);

  fdinfo = get_ioinfo_alloc_fd(MSVCRT_STDERR_FILENO);
  if (!(fdinfo->wxflag & WX_OPEN) || fdinfo->handle == INVALID_HANDLE_VALUE) {
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    DWORD type = GetFileType(h);

    msvcrt_set_fd(fdinfo, h, WX_OPEN|WX_TEXT|((type&0xf)==FILE_TYPE_CHAR ? WX_TTY : 0)
            |((type&0xf)==FILE_TYPE_PIPE ? WX_PIPE : 0));
  }
  release_ioinfo(fdinfo);

  TRACE(":handles (%p)(%p)(%p)\n", get_ioinfo_nolock(MSVCRT_STDIN_FILENO)->handle,
        get_ioinfo_nolock(MSVCRT_STDOUT_FILENO)->handle,
        get_ioinfo_nolock(MSVCRT_STDERR_FILENO)->handle);

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
    if((file->_flag & (MSVCRT__IOREAD|MSVCRT__IOWRT)) == MSVCRT__IOWRT &&
            file->_flag & (MSVCRT__IOMYBUF|MSVCRT__USERBUF)) {
        int cnt=file->_ptr-file->_base;
        if(cnt>0 && MSVCRT__write(file->_file, file->_base, cnt) != cnt) {
            file->_flag |= MSVCRT__IOERR;
            return MSVCRT_EOF;
        }

        if(file->_flag & MSVCRT__IORW)
            file->_flag &= ~MSVCRT__IOWRT;
    }

    file->_ptr=file->_base;
    file->_cnt=0;
    return 0;
}

/*********************************************************************
 *		_isatty (MSVCRT.@)
 */
int CDECL MSVCRT__isatty(int fd)
{
    TRACE(":fd (%d)\n",fd);

    return get_ioinfo_nolock(fd)->wxflag & WX_TTY;
}

/* INTERNAL: Allocate stdio file buffer */
static BOOL msvcrt_alloc_buffer(MSVCRT_FILE* file)
{
    if((file->_file==MSVCRT_STDOUT_FILENO || file->_file==MSVCRT_STDERR_FILENO)
            && MSVCRT__isatty(file->_file))
        return FALSE;

    file->_base = MSVCRT_calloc(1, MSVCRT_INTERNAL_BUFSIZ);
    if(file->_base) {
        file->_bufsiz = MSVCRT_INTERNAL_BUFSIZ;
        file->_flag |= MSVCRT__IOMYBUF;
    } else {
        file->_base = (char*)(&file->_charbuf);
        file->_bufsiz = 2;
        file->_flag |= MSVCRT__IONBF;
    }
    file->_ptr = file->_base;
    file->_cnt = 0;
    return TRUE;
}

/* INTERNAL: Allocate temporary buffer for stdout and stderr */
static BOOL add_std_buffer(MSVCRT_FILE *file)
{
    static char buffers[2][MSVCRT_BUFSIZ];

    if((file->_file!=MSVCRT_STDOUT_FILENO && file->_file!=MSVCRT_STDERR_FILENO)
            || (file->_flag & (MSVCRT__IONBF | MSVCRT__IOMYBUF | MSVCRT__USERBUF))
            || !MSVCRT__isatty(file->_file))
        return FALSE;

    file->_ptr = file->_base = buffers[file->_file == MSVCRT_STDOUT_FILENO ? 0 : 1];
    file->_bufsiz = file->_cnt = MSVCRT_BUFSIZ;
    file->_flag |= MSVCRT__USERBUF;
    return TRUE;
}

/* INTERNAL: Removes temporary buffer from stdout or stderr */
/* Only call this function when add_std_buffer returned TRUE */
static void remove_std_buffer(MSVCRT_FILE *file)
{
    msvcrt_flush_buffer(file);
    file->_ptr = file->_base = NULL;
    file->_bufsiz = file->_cnt = 0;
    file->_flag &= ~MSVCRT__USERBUF;
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
 *		__acrt_iob_func(MSVCRT.@)
 */
MSVCRT_FILE * CDECL MSVCRT___acrt_iob_func(unsigned idx)
{
 return &MSVCRT__iob[idx];
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
int CDECL MSVCRT__access_s(const char *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL)) return *MSVCRT__errno();
  if (!MSVCRT_CHECK_PMT((mode & ~(MSVCRT_R_OK | MSVCRT_W_OK)) == 0)) return *MSVCRT__errno();

  if (MSVCRT__access(filename, mode) == -1)
    return *MSVCRT__errno();
  return 0;
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
int CDECL MSVCRT__waccess_s(const MSVCRT_wchar_t *filename, int mode)
{
  if (!MSVCRT_CHECK_PMT(filename != NULL)) return *MSVCRT__errno();
  if (!MSVCRT_CHECK_PMT((mode & ~(MSVCRT_R_OK | MSVCRT_W_OK)) == 0)) return *MSVCRT__errno();

  if (MSVCRT__waccess(filename, mode) == -1)
    return *MSVCRT__errno();
  return 0;
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

/*********************************************************************
 *		_commit (MSVCRT.@)
 */
int CDECL MSVCRT__commit(int fd)
{
    ioinfo *info = get_ioinfo(fd);
    int ret;

    TRACE(":fd (%d) handle (%p)\n", fd, info->handle);

    if (info->handle == INVALID_HANDLE_VALUE)
        ret = -1;
    else if (!FlushFileBuffers(info->handle))
    {
        if (GetLastError() == ERROR_INVALID_HANDLE)
        {
            /* FlushFileBuffers fails for console handles
             * so we ignore this error.
             */
            ret = 0;
        }
        else
        {
            TRACE(":failed-last error (%d)\n",GetLastError());
            msvcrt_set_errno(GetLastError());
            ret = -1;
        }
    }
    else
    {
        TRACE(":ok\n");
        ret = 0;
    }

    release_ioinfo(info);
    return ret;
}

/* flush_all_buffers calls MSVCRT_fflush which calls flush_all_buffers */
int CDECL MSVCRT_fflush(MSVCRT_FILE* file);

/* INTERNAL: Flush all stream buffer */
static int msvcrt_flush_all_buffers(int mask)
{
  int i, num_flushed = 0;
  MSVCRT_FILE *file;

  LOCK_FILES();
  for (i = 0; i < MSVCRT_stream_idx; i++) {
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
    int ret;

    if(!file) {
        msvcrt_flush_all_buffers(MSVCRT__IOWRT);
        ret = 0;
    } else {
        MSVCRT__lock_file(file);
        ret = MSVCRT__fflush_nolock(file);
        MSVCRT__unlock_file(file);
    }

    return ret;
}

/*********************************************************************
 *		_fflush_nolock (MSVCRT.@)
 */
int CDECL MSVCRT__fflush_nolock(MSVCRT_FILE* file)
{
    int res;

    if(!file) {
        msvcrt_flush_all_buffers(MSVCRT__IOWRT);
        return 0;
    }

    res = msvcrt_flush_buffer(file);
    if(!res && (file->_flag & MSVCRT__IOCOMMIT))
        res = MSVCRT__commit(file->_file) ? MSVCRT_EOF : 0;
    return res;
}

/*********************************************************************
 *		_close (MSVCRT.@)
 */
int CDECL MSVCRT__close(int fd)
{
  ioinfo *info = get_ioinfo(fd);
  int ret;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);
  if (!(info->wxflag & WX_OPEN)) {
    ret = -1;
  } else if (fd == MSVCRT_STDOUT_FILENO &&
          info->handle == get_ioinfo_nolock(MSVCRT_STDERR_FILENO)->handle) {
    msvcrt_free_fd(fd);
    ret = 0;
  } else if (fd == MSVCRT_STDERR_FILENO &&
          info->handle == get_ioinfo_nolock(MSVCRT_STDOUT_FILENO)->handle) {
    msvcrt_free_fd(fd);
    ret = 0;
  } else {
    ret = CloseHandle(info->handle) ? 0 : -1;
    msvcrt_free_fd(fd);
    if (ret) {
      WARN(":failed-last error (%d)\n",GetLastError());
      msvcrt_set_errno(GetLastError());
    }
  }
  release_ioinfo(info);
  return ret;
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
  ioinfo *info_od, *info_nd;
  int ret;

  TRACE("(od=%d, nd=%d)\n", od, nd);

  if (od < nd)
  {
    info_od = get_ioinfo(od);
    info_nd = get_ioinfo_alloc_fd(nd);
  }
  else
  {
    info_nd = get_ioinfo_alloc_fd(nd);
    info_od = get_ioinfo(od);
  }

  if (info_nd == &MSVCRT___badioinfo)
  {
      ret = -1;
  }
  else if (info_od->wxflag & WX_OPEN)
  {
    HANDLE handle;

    if (DuplicateHandle(GetCurrentProcess(), info_od->handle,
     GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
      int wxflag = info_od->wxflag & ~MSVCRT__O_NOINHERIT;

      if (info_nd->wxflag & WX_OPEN)
        MSVCRT__close(nd);

      msvcrt_set_fd(info_nd, handle, wxflag);
      /* _dup2 returns 0, not nd, on success */
      ret = 0;
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

  release_ioinfo(info_od);
  release_ioinfo(info_nd);
  return ret;
}

/*********************************************************************
 *		_dup (MSVCRT.@)
 */
int CDECL MSVCRT__dup(int od)
{
  int fd, ret;
  ioinfo *info = get_ioinfo_alloc(&fd);
 
  if (MSVCRT__dup2(od, fd) == 0)
    ret = fd;
  else
    ret = -1;
  release_ioinfo(info);
  return ret;
}

/*********************************************************************
 *		_eof (MSVCRT.@)
 */
int CDECL MSVCRT__eof(int fd)
{
  ioinfo *info = get_ioinfo(fd);
  DWORD curpos,endpos;
  LONG hcurpos,hendpos;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);

  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (info->wxflag & WX_ATEOF)
  {
      release_ioinfo(info);
      return TRUE;
  }

  /* Otherwise we do it the hard way */
  hcurpos = hendpos = 0;
  curpos = SetFilePointer(info->handle, 0, &hcurpos, FILE_CURRENT);
  endpos = SetFilePointer(info->handle, 0, &hendpos, FILE_END);

  if (curpos == endpos && hcurpos == hendpos)
  {
    /* FIXME: shouldn't WX_ATEOF be set here? */
    release_ioinfo(info);
    return TRUE;
  }

  SetFilePointer(info->handle, curpos, &hcurpos, FILE_BEGIN);
  release_ioinfo(info);
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
    unsigned int i;
    int j;

    MSVCRT__flushall();
    MSVCRT__fcloseall();

    for(i=0; i<sizeof(MSVCRT___pioinfo)/sizeof(MSVCRT___pioinfo[0]); i++)
    {
        if(!MSVCRT___pioinfo[i])
            continue;

        for(j=0; j<MSVCRT_FD_BLOCK_SIZE; j++)
        {
            if(MSVCRT___pioinfo[i][j].exflag & EF_CRIT_INIT)
                DeleteCriticalSection(&MSVCRT___pioinfo[i][j].crit);
        }
        MSVCRT_free(MSVCRT___pioinfo[i]);
    }

    for(j=0; j<MSVCRT_stream_idx; j++)
    {
        MSVCRT_FILE *file = msvcrt_get_file(j);
        if(file<MSVCRT__iob || file>=MSVCRT__iob+_IOB_ENTRIES)
        {
            ((file_crit*)file)->crit.DebugInfo->Spare[0] = 0;
            DeleteCriticalSection(&((file_crit*)file)->crit);
        }
    }

    for(i=0; i<sizeof(MSVCRT_fstream)/sizeof(MSVCRT_fstream[0]); i++)
        MSVCRT_free(MSVCRT_fstream[i]);
}

/*********************************************************************
 *		_lseeki64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__lseeki64(int fd, __int64 offset, int whence)
{
  ioinfo *info = get_ioinfo(fd);
  LARGE_INTEGER ofs;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);

  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (whence < 0 || whence > 2)
  {
    release_ioinfo(info);
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  TRACE(":fd (%d) to %s pos %s\n",
        fd,wine_dbgstr_longlong(offset),
        (whence==MSVCRT_SEEK_SET)?"SEEK_SET":
        (whence==MSVCRT_SEEK_CUR)?"SEEK_CUR":
        (whence==MSVCRT_SEEK_END)?"SEEK_END":"UNKNOWN");

  /* The MoleBox protection scheme expects msvcrt to use SetFilePointer only,
   * so a LARGE_INTEGER offset cannot be passed directly via SetFilePointerEx. */
  ofs.QuadPart = offset;
  if ((ofs.u.LowPart = SetFilePointer(info->handle, ofs.u.LowPart, &ofs.u.HighPart, whence)) != INVALID_SET_FILE_POINTER ||
      GetLastError() == ERROR_SUCCESS)
  {
    info->wxflag &= ~WX_ATEOF;
    /* FIXME: What if we seek _to_ EOF - is EOF set? */

    release_ioinfo(info);
    return ofs.QuadPart;
  }
  release_ioinfo(info);
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
  ioinfo *info = get_ioinfo(fd);
  BOOL ret;
  DWORD cur_locn;

  TRACE(":fd (%d) handle (%p)\n", fd, info->handle);
  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (mode < 0 || mode > 4)
  {
    release_ioinfo(info);
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

  if ((cur_locn = SetFilePointer(info->handle, 0L, NULL, FILE_CURRENT)) == INVALID_SET_FILE_POINTER)
  {
    release_ioinfo(info);
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
      ret = LockFile(info->handle, cur_locn, 0L, nbytes, 0L);
      if (ret) break;
      Sleep(1);
    }
  }
  else if (mode == MSVCRT__LK_UNLCK)
    ret = UnlockFile(info->handle, cur_locn, 0L, nbytes, 0L);
  else
    ret = LockFile(info->handle, cur_locn, 0L, nbytes, 0L);
  /* FIXME - what about error settings? */
  release_ioinfo(info);
  return ret ? 0 : -1;
}

/*********************************************************************
 *		_fseeki64 (MSVCRT.@)
 */
int CDECL MSVCRT__fseeki64(MSVCRT_FILE* file, __int64 offset, int whence)
{
    int ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fseeki64_nolock(file, offset, whence);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fseeki64_nolock (MSVCRT.@)
 */
int CDECL MSVCRT__fseeki64_nolock(MSVCRT_FILE* file, __int64 offset, int whence)
{
  int ret;

  if(whence == MSVCRT_SEEK_CUR && file->_flag & MSVCRT__IOREAD ) {
      whence = MSVCRT_SEEK_SET;
      offset += MSVCRT__ftelli64_nolock(file);
  }

  /* Flush output if needed */
  msvcrt_flush_buffer(file);
  /* Reset direction of i/o */
  if(file->_flag & MSVCRT__IORW) {
        file->_flag &= ~(MSVCRT__IOREAD|MSVCRT__IOWRT);
  }
  /* Clear end of file flag */
  file->_flag &= ~MSVCRT__IOEOF;
  ret = (MSVCRT__lseeki64(file->_file,offset,whence) == -1)?-1:0;

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
 *		_fseek_nolock (MSVCRT.@)
 */
int CDECL MSVCRT__fseek_nolock(MSVCRT_FILE* file, MSVCRT_long offset, int whence)
{
    return MSVCRT__fseeki64_nolock( file, offset, whence );
}

/*********************************************************************
 *		_chsize_s (MSVCRT.@)
 */
int CDECL MSVCRT__chsize_s(int fd, __int64 size)
{
    ioinfo *info;
    __int64 cur, pos;
    BOOL ret = FALSE;

    TRACE("(fd=%d, size=%s)\n", fd, wine_dbgstr_longlong(size));

    if (!MSVCRT_CHECK_PMT(size >= 0)) return MSVCRT_EINVAL;


    info = get_ioinfo(fd);
    if (info->handle != INVALID_HANDLE_VALUE)
    {
        /* save the current file pointer */
        cur = MSVCRT__lseeki64(fd, 0, MSVCRT_SEEK_CUR);
        if (cur >= 0)
        {
            pos = MSVCRT__lseeki64(fd, size, MSVCRT_SEEK_SET);
            if (pos >= 0)
            {
                ret = SetEndOfFile(info->handle);
                if (!ret) msvcrt_set_errno(GetLastError());
            }

            /* restore the file pointer */
            MSVCRT__lseeki64(fd, cur, MSVCRT_SEEK_SET);
        }
    }

    release_ioinfo(info);
    return ret ? 0 : *MSVCRT__errno();
}

/*********************************************************************
 *		_chsize (MSVCRT.@)
 */
int CDECL MSVCRT__chsize(int fd, MSVCRT_long size)
{
    /* _chsize_s returns errno on failure but _chsize should return -1 */
    return MSVCRT__chsize_s( fd, size ) == 0 ? 0 : -1;
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
  MSVCRT__fseek_nolock(file, 0L, MSVCRT_SEEK_SET);
  MSVCRT_clearerr(file);
  MSVCRT__unlock_file(file);
}

static int msvcrt_get_flags(const MSVCRT_wchar_t* mode, int *open_flags, int* stream_flags)
{
  int plus = strchrW(mode, '+') != NULL;

  TRACE("%s\n", debugstr_w(mode));

  while(*mode == ' ') mode++;

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
    MSVCRT_INVALID_PMT(0, MSVCRT_EINVAL);
    return -1;
  }

  *stream_flags |= MSVCRT__commode;

  while (*mode && *mode!=',')
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
    case 'c':
      *stream_flags |= MSVCRT__IOCOMMIT;
      break;
    case 'n':
      *stream_flags &= ~MSVCRT__IOCOMMIT;
      break;
    case 'N':
      *open_flags |= MSVCRT__O_NOINHERIT;
      break;
    case '+':
    case ' ':
    case 'a':
    case 'w':
      break;
    case 'S':
    case 'R':
      FIXME("ignoring cache optimization flag: %c\n", mode[-1]);
      break;
    default:
      ERR("incorrect mode flag: %c\n", mode[-1]);
      break;
    }

  if(*mode == ',')
  {
    static const WCHAR ccs[] = {'c','c','s'};
    static const WCHAR utf8[] = {'u','t','f','-','8'};
    static const WCHAR utf16le[] = {'u','t','f','-','1','6','l','e'};
    static const WCHAR unicode[] = {'u','n','i','c','o','d','e'};

    mode++;
    while(*mode == ' ') mode++;
    if(!MSVCRT_CHECK_PMT(!strncmpW(ccs, mode, sizeof(ccs)/sizeof(ccs[0]))))
      return -1;
    mode += sizeof(ccs)/sizeof(ccs[0]);
    while(*mode == ' ') mode++;
    if(!MSVCRT_CHECK_PMT(*mode == '='))
        return -1;
    mode++;
    while(*mode == ' ') mode++;

    if(!strncmpiW(utf8, mode, sizeof(utf8)/sizeof(utf8[0])))
    {
      *open_flags |= MSVCRT__O_U8TEXT;
      mode += sizeof(utf8)/sizeof(utf8[0]);
    }
    else if(!strncmpiW(utf16le, mode, sizeof(utf16le)/sizeof(utf16le[0])))
    {
      *open_flags |= MSVCRT__O_U16TEXT;
      mode += sizeof(utf16le)/sizeof(utf16le[0]);
    }
    else if(!strncmpiW(unicode, mode, sizeof(unicode)/sizeof(unicode[0])))
    {
      *open_flags |= MSVCRT__O_WTEXT;
      mode += sizeof(unicode)/sizeof(unicode[0]);
    }
    else
    {
      MSVCRT_INVALID_PMT(0, MSVCRT_EINVAL);
      return -1;
    }

    while(*mode == ' ') mode++;
  }

  if(!MSVCRT_CHECK_PMT(*mode == 0))
    return -1;
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
  LONG curPos = MSVCRT__lseek(fd, 0, MSVCRT_SEEK_CUR);
  if (curPos != -1)
  {
    LONG endPos = MSVCRT__lseek(fd, 0, MSVCRT_SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        MSVCRT__lseek(fd, curPos, MSVCRT_SEEK_SET);
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
  __int64 curPos = MSVCRT__lseeki64(fd, 0, MSVCRT_SEEK_CUR);
  if (curPos != -1)
  {
    __int64 endPos = MSVCRT__lseeki64(fd, 0, MSVCRT_SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        MSVCRT__lseeki64(fd, curPos, MSVCRT_SEEK_SET);
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
  ioinfo *info = get_ioinfo(fd);
  DWORD dw;
  DWORD type;
  BY_HANDLE_FILE_INFORMATION hfi;

  TRACE(":fd (%d) stat (%p)\n", fd, buf);
  if (info->handle == INVALID_HANDLE_VALUE)
  {
    release_ioinfo(info);
    return -1;
  }

  if (!buf)
  {
    WARN(":failed-NULL buf\n");
    msvcrt_set_errno(ERROR_INVALID_PARAMETER);
    release_ioinfo(info);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct MSVCRT__stat64));
  type = GetFileType(info->handle);
  if (type == FILE_TYPE_PIPE)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = MSVCRT__S_IFIFO;
    buf->st_nlink = 1;
  }
  else if (type == FILE_TYPE_CHAR)
  {
    buf->st_dev = buf->st_rdev = fd;
    buf->st_mode = MSVCRT__S_IFCHR;
    buf->st_nlink = 1;
  }
  else /* FILE_TYPE_DISK etc. */
  {
    if (!GetFileInformationByHandle(info->handle, &hfi))
    {
      WARN(":failed-last error (%d)\n",GetLastError());
      msvcrt_set_errno(ERROR_INVALID_PARAMETER);
      release_ioinfo(info);
      return -1;
    }
    buf->st_mode = MSVCRT__S_IFREG | 0444;
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
  release_ioinfo(info);
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
 *		_fstat32 (MSVCR80.@)
 */
int CDECL MSVCRT__fstat32(int fd, struct MSVCRT__stat32* buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT__fstat64(fd, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_fstat32i64 (MSVCR80.@)
 */
int CDECL MSVCRT__fstat32i64(int fd, struct MSVCRT__stat32i64* buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT__fstat64(fd, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32i64(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_fstat64i32 (MSVCR80.@)
 */
int CDECL MSVCRT__fstat64i32(int fd, struct MSVCRT__stat64i32* buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT__fstat64(fd, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_futime64 (MSVCRT.@)
 */
int CDECL _futime64(int fd, struct MSVCRT___utimbuf64 *t)
{
  ioinfo *info = get_ioinfo(fd);
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

  if (!SetFileTime(info->handle, NULL, &at, &wt))
  {
    release_ioinfo(info);
    msvcrt_set_errno(GetLastError());
    return -1 ;
  }
  release_ioinfo(info);
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
  HANDLE hand = get_ioinfo_nolock(fd)->handle;
  TRACE(":fd (%d) handle (%p)\n",fd,hand);

  if(hand == INVALID_HANDLE_VALUE)
      *MSVCRT__errno() = MSVCRT_EBADF;
  return (MSVCRT_intptr_t)hand;
}

/*********************************************************************
 *		_mktemp_s (MSVCRT.@)
 */
int CDECL MSVCRT__mktemp_s(char *pattern, MSVCRT_size_t size)
{
    DWORD len, xno, id;

    if(!MSVCRT_CHECK_PMT(pattern!=NULL))
        return MSVCRT_EINVAL;

    for(len=0; len<size; len++)
        if(!pattern[len])
            break;
    if(!MSVCRT_CHECK_PMT(len!=size && len>=6)) {
        if(size)
            pattern[0] = 0;
        return MSVCRT_EINVAL;
    }

    for(xno=1; xno<=6; xno++)
        if(!MSVCRT_CHECK_PMT(pattern[len-xno] == 'X'))
            return MSVCRT_EINVAL;

    id = GetCurrentProcessId();
    for(xno=1; xno<6; xno++) {
        pattern[len-xno] = id%10 + '0';
        id /= 10;
    }

    for(pattern[len-6]='a'; pattern[len-6]<='z'; pattern[len-6]++) {
        if(GetFileAttributesA(pattern) == INVALID_FILE_ATTRIBUTES)
            return 0;
    }

    pattern[0] = 0;
    *MSVCRT__errno() = MSVCRT_EEXIST;
    return MSVCRT_EEXIST;
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

  if(!pattern)
      return NULL;

  while(*pattern)
    numX = (*pattern++ == 'X')? numX + 1 : 0;
  if (numX < 6)
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
    if (GetFileAttributesA(retVal) == INVALID_FILE_ATTRIBUTES)
      return retVal;
  } while(letter <= 'z');
  return NULL;
}

/*********************************************************************
 *		_wmktemp_s (MSVCRT.@)
 */
int CDECL MSVCRT__wmktemp_s(MSVCRT_wchar_t *pattern, MSVCRT_size_t size)
{
    DWORD len, xno, id;

    if(!MSVCRT_CHECK_PMT(pattern!=NULL))
        return MSVCRT_EINVAL;

    for(len=0; len<size; len++)
        if(!pattern[len])
            break;
    if(!MSVCRT_CHECK_PMT(len!=size && len>=6)) {
        if(size)
            pattern[0] = 0;
        return MSVCRT_EINVAL;
    }

    for(xno=1; xno<=6; xno++)
        if(!MSVCRT_CHECK_PMT(pattern[len-xno] == 'X'))
            return MSVCRT_EINVAL;

    id = GetCurrentProcessId();
    for(xno=1; xno<6; xno++) {
        pattern[len-xno] = id%10 + '0';
        id /= 10;
    }

    for(pattern[len-6]='a'; pattern[len-6]<='z'; pattern[len-6]++) {
        if(GetFileAttributesW(pattern) == INVALID_FILE_ATTRIBUTES)
            return 0;
    }

    pattern[0] = 0;
    *MSVCRT__errno() = MSVCRT_EEXIST;
    return MSVCRT_EEXIST;
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

  if(!pattern)
      return NULL;

  while(*pattern)
    numX = (*pattern++ == 'X')? numX + 1 : 0;
  if (numX < 6)
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
    if (GetFileAttributesW(retVal) == INVALID_FILE_ATTRIBUTES)
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
    else if (oflags & MSVCRT__O_WTEXT)          wxflags |= WX_TEXT;
    else if (oflags & MSVCRT__O_U16TEXT)        wxflags |= WX_TEXT;
    else if (oflags & MSVCRT__O_U8TEXT)         wxflags |= WX_TEXT;
    else if (*__p__fmode() & MSVCRT__O_BINARY)  {/* Nothing to do */}
    else                                        wxflags |= WX_TEXT; /* default to TEXT*/
    if (oflags & MSVCRT__O_NOINHERIT)           wxflags |= WX_DONTINHERIT;

    if ((unsupp = oflags & ~(
                    MSVCRT__O_BINARY|MSVCRT__O_TEXT|MSVCRT__O_APPEND|
                    MSVCRT__O_TRUNC|MSVCRT__O_EXCL|MSVCRT__O_CREAT|
                    MSVCRT__O_RDWR|MSVCRT__O_WRONLY|MSVCRT__O_TEMPORARY|
                    MSVCRT__O_NOINHERIT|
                    MSVCRT__O_SEQUENTIAL|MSVCRT__O_RANDOM|MSVCRT__O_SHORT_LIVED|
                    MSVCRT__O_WTEXT|MSVCRT__O_U16TEXT|MSVCRT__O_U8TEXT
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

    fd = msvcrt_alloc_fd(readHandle, wxflags|WX_PIPE);
    if (fd != -1)
    {
      pfds[0] = fd;
      fd = msvcrt_alloc_fd(writeHandle, wxflags|WX_PIPE);
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
  }
  else
    msvcrt_set_errno(GetLastError());

  return ret;
}

static int check_bom(HANDLE h, int oflags, BOOL seek)
{
    char bom[sizeof(utf8_bom)];
    DWORD r;

    oflags &= ~(MSVCRT__O_WTEXT|MSVCRT__O_U16TEXT|MSVCRT__O_U8TEXT);

    if (!ReadFile(h, bom, sizeof(utf8_bom), &r, NULL))
        return oflags;

    if (r==sizeof(utf8_bom) && !memcmp(bom, utf8_bom, sizeof(utf8_bom))) {
        oflags |= MSVCRT__O_U8TEXT;
    }else if (r>=sizeof(utf16_bom) && !memcmp(bom, utf16_bom, sizeof(utf16_bom))) {
        if (seek && r>2)
            SetFilePointer(h, 2, NULL, FILE_BEGIN);
        oflags |= MSVCRT__O_U16TEXT;
    }else if (seek) {
        SetFilePointer(h, 0, NULL, FILE_BEGIN);
    }

    return oflags;
}

/*********************************************************************
 *              _wsopen_s (MSVCRT.@)
 */
int CDECL MSVCRT__wsopen_s( int *fd, const MSVCRT_wchar_t* path, int oflags, int shflags, int pmode )
{
  DWORD access = 0, creation = 0, attrib;
  SECURITY_ATTRIBUTES sa;
  DWORD sharing, type;
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

  if (!(pmode & ~MSVCRT_umask & MSVCRT__S_IWRITE))
      attrib = FILE_ATTRIBUTE_READONLY;
  else
      attrib = FILE_ATTRIBUTE_NORMAL;

  if (oflags & MSVCRT__O_TEMPORARY)
  {
      attrib |= FILE_FLAG_DELETE_ON_CLOSE;
      access |= DELETE;
      sharing |= FILE_SHARE_DELETE;
  }

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = !(oflags & MSVCRT__O_NOINHERIT);

  if ((oflags&(MSVCRT__O_WTEXT|MSVCRT__O_U16TEXT|MSVCRT__O_U8TEXT))
          && (creation==OPEN_ALWAYS || creation==OPEN_EXISTING)
          && !(access&GENERIC_READ))
  {
      hand = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
              &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (hand != INVALID_HANDLE_VALUE)
      {
          oflags = check_bom(hand, oflags, FALSE);
          CloseHandle(hand);
      }
      else
          oflags &= ~(MSVCRT__O_WTEXT|MSVCRT__O_U16TEXT|MSVCRT__O_U8TEXT);
  }

  hand = CreateFileW(path, access, sharing, &sa, creation, attrib, 0);
  if (hand == INVALID_HANDLE_VALUE)  {
    WARN(":failed-last error (%d)\n",GetLastError());
    msvcrt_set_errno(GetLastError());
    return *MSVCRT__errno();
  }

  if (oflags & (MSVCRT__O_WTEXT|MSVCRT__O_U16TEXT|MSVCRT__O_U8TEXT))
  {
      if ((access & GENERIC_WRITE) && (creation==CREATE_NEW
                  || creation==CREATE_ALWAYS || creation==TRUNCATE_EXISTING
                  || (creation==OPEN_ALWAYS && GetLastError()==ERROR_ALREADY_EXISTS)))
      {
          if (oflags & MSVCRT__O_U8TEXT)
          {
              DWORD written = 0, tmp;

              while(written!=sizeof(utf8_bom) && WriteFile(hand, (char*)utf8_bom+written,
                          sizeof(utf8_bom)-written, &tmp, NULL))
                  written += tmp;
              if (written != sizeof(utf8_bom)) {
                  WARN("error writing BOM\n");
                  CloseHandle(hand);
                  msvcrt_set_errno(GetLastError());
                  return *MSVCRT__errno();
              }
          }
          else
          {
              DWORD written = 0, tmp;

              while(written!=sizeof(utf16_bom) && WriteFile(hand, (char*)utf16_bom+written,
                          sizeof(utf16_bom)-written, &tmp, NULL))
                  written += tmp;
              if (written != sizeof(utf16_bom))
              {
                  WARN("error writing BOM\n");
                  CloseHandle(hand);
                  msvcrt_set_errno(GetLastError());
                  return *MSVCRT__errno();
              }
          }
      }
      else if (access & GENERIC_READ)
          oflags = check_bom(hand, oflags, TRUE);
  }

  type = GetFileType(hand);
  if (type == FILE_TYPE_CHAR)
      wxflag |= WX_TTY;
  else if (type == FILE_TYPE_PIPE)
      wxflag |= WX_PIPE;

  *fd = msvcrt_alloc_fd(hand, wxflag);
  if (*fd == -1)
      return *MSVCRT__errno();

  if (oflags & MSVCRT__O_WTEXT)
      get_ioinfo_nolock(*fd)->exflag |= EF_UTF16|EF_UNK_UNICODE;
  else if (oflags & MSVCRT__O_U16TEXT)
      get_ioinfo_nolock(*fd)->exflag |= EF_UTF16;
  else if (oflags & MSVCRT__O_U8TEXT)
      get_ioinfo_nolock(*fd)->exflag |= EF_UTF8;

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
 *              _sopen_s (MSVCRT.@)
 */
int CDECL MSVCRT__sopen_s( int *fd, const char *path, int oflags, int shflags, int pmode )
{
    MSVCRT_wchar_t *pathW;
    int ret;

    if (!MSVCRT_CHECK_PMT(fd != NULL))
        return MSVCRT_EINVAL;
    *fd = -1;
    if(!MSVCRT_CHECK_PMT(path && (pathW = msvcrt_wstrdupa(path))))
        return MSVCRT_EINVAL;

    ret = MSVCRT__wsopen_s(fd, pathW, oflags, shflags, pmode);
    MSVCRT_free(pathW);
    return ret;
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
  DWORD flags;
  int fd;

  /* MSVCRT__O_RDONLY (0) always matches, so set the read flag
   * MFC's CStdioFile clears O_RDONLY (0)! if it wants to write to the
   * file, so set the write flag. It also only sets MSVCRT__O_TEXT if it wants
   * text - it never sets MSVCRT__O_BINARY.
   */
  /* don't let split_oflags() decide the mode if no mode is passed */
  if (!(oflags & (MSVCRT__O_BINARY | MSVCRT__O_TEXT)))
      oflags |= MSVCRT__O_BINARY;

  flags = GetFileType((HANDLE)handle);
  if (flags==FILE_TYPE_UNKNOWN && GetLastError()!=NO_ERROR)
  {
    msvcrt_set_errno(GetLastError());
    return -1;
  }

  if (flags == FILE_TYPE_CHAR)
    flags = WX_TTY;
  else if (flags == FILE_TYPE_PIPE)
    flags = WX_PIPE;
  else
    flags = 0;
  flags |= split_oflags(oflags);

  fd = msvcrt_alloc_fd((HANDLE)handle, flags);
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n", handle, fd, flags);
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

static inline int get_utf8_char_len(char ch)
{
    if((ch&0xf8) == 0xf0)
        return 4;
    else if((ch&0xf0) == 0xe0)
        return 3;
    else if((ch&0xe0) == 0xc0)
        return 2;
    return 1;
}

/*********************************************************************
 * (internal) read_utf8
 */
static int read_utf8(ioinfo *fdinfo, MSVCRT_wchar_t *buf, unsigned int count)
{
    HANDLE hand = fdinfo->handle;
    char min_buf[4], *readbuf, lookahead;
    DWORD readbuf_size, pos=0, num_read=1, char_len, i, j;

    /* make the buffer big enough to hold at least one character */
    /* read bytes have to fit to output and lookahead buffers */
    count /= 2;
    readbuf_size = count < 4 ? 4 : count;
    if(readbuf_size<=4 || !(readbuf = MSVCRT_malloc(readbuf_size))) {
        readbuf_size = 4;
        readbuf = min_buf;
    }

    if(fdinfo->lookahead[0] != '\n') {
        readbuf[pos++] = fdinfo->lookahead[0];
        fdinfo->lookahead[0] = '\n';

        if(fdinfo->lookahead[1] != '\n') {
            readbuf[pos++] = fdinfo->lookahead[1];
            fdinfo->lookahead[1] = '\n';

            if(fdinfo->lookahead[2] != '\n') {
                readbuf[pos++] = fdinfo->lookahead[2];
                fdinfo->lookahead[2] = '\n';
            }
        }
    }

    /* NOTE: this case is broken in native dll, reading
     *        sometimes fails when small buffer is passed
     */
    if(count < 4) {
        if(!pos && !ReadFile(hand, readbuf, 1, &num_read, NULL)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                fdinfo->wxflag |= WX_ATEOF;
                return 0;
            }else {
                msvcrt_set_errno(GetLastError());
                return -1;
            }
        }else if(!num_read) {
            fdinfo->wxflag |= WX_ATEOF;
            return 0;
        }else {
            pos++;
        }

        char_len = get_utf8_char_len(readbuf[0]);
        if(char_len>pos) {
            if(ReadFile(hand, readbuf+pos, char_len-pos, &num_read, NULL))
                pos += num_read;
        }

        if(readbuf[0] == '\n')
            fdinfo->wxflag |= WX_READNL;
        else
            fdinfo->wxflag &= ~WX_READNL;

        if(readbuf[0] == 0x1a) {
            fdinfo->wxflag |= WX_ATEOF;
            return 0;
        }

        if(readbuf[0] == '\r') {
            if(!ReadFile(hand, &lookahead, 1, &num_read, NULL) || num_read!=1)
                buf[0] = '\r';
            else if(lookahead == '\n')
                buf[0] = '\n';
            else {
                buf[0] = '\r';
                if(fdinfo->wxflag & (WX_PIPE | WX_TTY))
                    fdinfo->lookahead[0] = lookahead;
                else
                    SetFilePointer(fdinfo->handle, -1, NULL, FILE_CURRENT);
            }
            return 2;
        }

        if(!(num_read = MultiByteToWideChar(CP_UTF8, 0, readbuf, pos, buf, count))) {
            msvcrt_set_errno(GetLastError());
            return -1;
        }

        return num_read*2;
    }

    if(!ReadFile(hand, readbuf+pos, readbuf_size-pos, &num_read, NULL)) {
        if(pos) {
            num_read = 0;
        }else if(GetLastError() == ERROR_BROKEN_PIPE) {
            fdinfo->wxflag |= WX_ATEOF;
            if (readbuf != min_buf) MSVCRT_free(readbuf);
            return 0;
        }else {
            msvcrt_set_errno(GetLastError());
            if (readbuf != min_buf) MSVCRT_free(readbuf);
            return -1;
        }
    }else if(!pos && !num_read) {
        fdinfo->wxflag |= WX_ATEOF;
        if (readbuf != min_buf) MSVCRT_free(readbuf);
        return 0;
    }

    pos += num_read;
    if(readbuf[0] == '\n')
        fdinfo->wxflag |= WX_READNL;
    else
        fdinfo->wxflag &= ~WX_READNL;

    /* Find first byte of last character (may be incomplete) */
    for(i=pos-1; i>0 && i>pos-4; i--)
        if((readbuf[i]&0xc0) != 0x80)
            break;
    char_len = get_utf8_char_len(readbuf[i]);
    if(char_len+i <= pos)
        i += char_len;

    if(fdinfo->wxflag & (WX_PIPE | WX_TTY)) {
        if(i < pos)
            fdinfo->lookahead[0] = readbuf[i];
        if(i+1 < pos)
            fdinfo->lookahead[1] = readbuf[i+1];
        if(i+2 < pos)
            fdinfo->lookahead[2] = readbuf[i+2];
    }else if(i < pos) {
        SetFilePointer(fdinfo->handle, i-pos, NULL, FILE_CURRENT);
    }
    pos = i;

    for(i=0, j=0; i<pos; i++) {
        if(readbuf[i] == 0x1a) {
            fdinfo->wxflag |= WX_ATEOF;
            break;
        }

        /* strip '\r' if followed by '\n' */
        if(readbuf[i] == '\r' && i+1==pos) {
            if(fdinfo->lookahead[0] != '\n' || !ReadFile(hand, &lookahead, 1, &num_read, NULL) || !num_read) {
                readbuf[j++] = '\r';
            }else if(lookahead == '\n' && j==0) {
                readbuf[j++] = '\n';
            }else {
                if(lookahead != '\n')
                    readbuf[j++] = '\r';

                if(fdinfo->wxflag & (WX_PIPE | WX_TTY))
                    fdinfo->lookahead[0] = lookahead;
                else
                    SetFilePointer(fdinfo->handle, -1, NULL, FILE_CURRENT);
            }
        }else if(readbuf[i]!='\r' || readbuf[i+1]!='\n') {
            readbuf[j++] = readbuf[i];
        }
    }
    pos = j;

    if(!(num_read = MultiByteToWideChar(CP_UTF8, 0, readbuf, pos, buf, count))) {
        msvcrt_set_errno(GetLastError());
        if (readbuf != min_buf) MSVCRT_free(readbuf);
        return -1;
    }

    if (readbuf != min_buf) MSVCRT_free(readbuf);
    return num_read*2;
}

/*********************************************************************
 * (internal) read_i
 *
 * When reading \r as last character in text mode, read() positions
 * the file pointer on the \r character while getc() goes on to
 * the following \n
 */
static int read_i(int fd, ioinfo *fdinfo, void *buf, unsigned int count)
{
    DWORD num_read, utf16;
    char *bufstart = buf;

    if (count == 0)
        return 0;

    if (fdinfo->wxflag & WX_ATEOF) {
        TRACE("already at EOF, returning 0\n");
        return 0;
    }
    /* Don't trace small reads, it gets *very* annoying */
    if (count > 4)
        TRACE(":fd (%d) handle (%p) buf (%p) len (%d)\n", fd, fdinfo->handle, buf, count);
    if (fdinfo->handle == INVALID_HANDLE_VALUE)
    {
        *MSVCRT__errno() = MSVCRT_EBADF;
        return -1;
    }

    utf16 = (fdinfo->exflag & EF_UTF16) != 0;
    if (((fdinfo->exflag&EF_UTF8) || utf16) && count&1)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return -1;
    }

    if((fdinfo->wxflag&WX_TEXT) && (fdinfo->exflag&EF_UTF8))
        return read_utf8(fdinfo, buf, count);

    if (fdinfo->lookahead[0]!='\n' || ReadFile(fdinfo->handle, bufstart, count, &num_read, NULL))
    {
        if (fdinfo->lookahead[0] != '\n')
        {
            bufstart[0] = fdinfo->lookahead[0];
            fdinfo->lookahead[0] = '\n';

            if (utf16)
            {
                bufstart[1] =  fdinfo->lookahead[1];
                fdinfo->lookahead[1] = '\n';
            }

            if(count>1+utf16 && ReadFile(fdinfo->handle, bufstart+1+utf16, count-1-utf16, &num_read, NULL))
                num_read += 1+utf16;
            else
                num_read = 1+utf16;
        }

        if(utf16 && (num_read&1))
        {
            /* msvcr90 uses uninitialized value from the buffer in this case */
            /* msvcrt ignores additional data */
            ERR("got odd number of bytes in UTF16 mode\n");
            num_read--;
        }

        if (count != 0 && num_read == 0)
        {
            fdinfo->wxflag |= WX_ATEOF;
            TRACE(":EOF %s\n",debugstr_an(buf,num_read));
        }
        else if (fdinfo->wxflag & WX_TEXT)
        {
            DWORD i, j;

            if (bufstart[0]=='\n' && (!utf16 || bufstart[1]==0))
                fdinfo->wxflag |= WX_READNL;
            else
                fdinfo->wxflag &= ~WX_READNL;

            for (i=0, j=0; i<num_read; i+=1+utf16)
            {
                /* in text mode, a ctrl-z signals EOF */
                if (bufstart[i]==0x1a && (!utf16 || bufstart[i+1]==0))
                {
                    fdinfo->wxflag |= WX_ATEOF;
                    TRACE(":^Z EOF %s\n",debugstr_an(buf,num_read));
                    break;
                }

                /* in text mode, strip \r if followed by \n */
                if (bufstart[i]=='\r' && (!utf16 || bufstart[i+1]==0) && i+1+utf16==num_read)
                {
                    char lookahead[2];
                    DWORD len;

                    lookahead[1] = '\n';
                    if (ReadFile(fdinfo->handle, lookahead, 1+utf16, &len, NULL) && len)
                    {
                        if(lookahead[0]=='\n' && (!utf16 || lookahead[1]==0) && j==0)
                        {
                            bufstart[j++] = '\n';
                            if(utf16) bufstart[j++] = 0;
                        }
                        else
                        {
                            if(lookahead[0]!='\n' || (utf16 && lookahead[1]!=0))
                            {
                                bufstart[j++] = '\r';
                                if(utf16) bufstart[j++] = 0;
                            }

                            if (fdinfo->wxflag & (WX_PIPE | WX_TTY))
                            {
                                if (lookahead[0]=='\n' && (!utf16 || !lookahead[1]))
                                {
                                    bufstart[j++] = '\n';
                                    if (utf16) bufstart[j++] = 0;
                                }
                                else
                                {
                                    fdinfo->lookahead[0] = lookahead[0];
                                    fdinfo->lookahead[1] = lookahead[1];
                                }
                            }
                            else
                                SetFilePointer(fdinfo->handle, -1-utf16, NULL, FILE_CURRENT);
                        }
                    }
                    else
                    {
                        bufstart[j++] = '\r';
                        if(utf16) bufstart[j++] = 0;
                    }
                }
                else if((bufstart[i]!='\r' || (utf16 && bufstart[i+1]!=0))
                        || (bufstart[i+1+utf16]!='\n' || (utf16 && bufstart[i+3]!=0)))
                {
                    bufstart[j++] = bufstart[i];
                    if(utf16) bufstart[j++] = bufstart[i+1];
                }
            }
            num_read = j;
        }
    }
    else
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            TRACE(":end-of-pipe\n");
            fdinfo->wxflag |= WX_ATEOF;
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
    ioinfo *info = get_ioinfo(fd);
    int num_read = read_i(fd, info, buf, count);
    release_ioinfo(info);
    return num_read;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int CDECL MSVCRT__setmode(int fd,int mode)
{
    ioinfo *info = get_ioinfo(fd);
    int ret = info->wxflag & WX_TEXT ? MSVCRT__O_TEXT : MSVCRT__O_BINARY;
    if(ret==MSVCRT__O_TEXT && (info->exflag & (EF_UTF8|EF_UTF16)))
        ret = MSVCRT__O_WTEXT;

    if(mode!=MSVCRT__O_TEXT && mode!=MSVCRT__O_BINARY && mode!=MSVCRT__O_WTEXT
                && mode!=MSVCRT__O_U16TEXT && mode!=MSVCRT__O_U8TEXT) {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        release_ioinfo(info);
        return -1;
    }

    if(info == &MSVCRT___badioinfo) {
        *MSVCRT__errno() = MSVCRT_EBADF;
        return MSVCRT_EOF;
    }

    if(mode == MSVCRT__O_BINARY) {
        info->wxflag &= ~WX_TEXT;
        info->exflag &= ~(EF_UTF8|EF_UTF16);
        release_ioinfo(info);
        return ret;
    }

    info->wxflag |= WX_TEXT;
    if(mode == MSVCRT__O_TEXT)
        info->exflag &= ~(EF_UTF8|EF_UTF16);
    else if(mode == MSVCRT__O_U8TEXT)
        info->exflag = (info->exflag & ~EF_UTF16) | EF_UTF8;
    else
        info->exflag = (info->exflag & ~EF_UTF8) | EF_UTF16;

    release_ioinfo(info);
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

  plen = strlen(path);
  while (plen && path[plen-1]==' ')
    plen--;

  if (plen && (plen<2 || path[plen-2]!=':') &&
          (path[plen-1]==':' || path[plen-1]=='\\' || path[plen-1]=='/'))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      *MSVCRT__errno() = MSVCRT_ENOENT;
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

  /* Dir, or regular file? */
  if (hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
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
{
  int ret;
  struct MSVCRT__stat64 buf64;

  ret = MSVCRT_stat64( path, &buf64);
  if (!ret)
      msvcrt_stat64_to_stat(&buf64, buf);
  return ret;
}

/*********************************************************************
 *  _stat32 (MSVCR100.@)
 */
int CDECL MSVCRT__stat32(const char *path, struct MSVCRT__stat32 *buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT_stat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *  _stat32i64 (MSVCR100.@)
 */
int CDECL MSVCRT__stat32i64(const char *path, struct MSVCRT__stat32i64 *buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT_stat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32i64(&buf64, buf);
    return ret;
}

/*********************************************************************
 * _stat64i32 (MSVCR100.@)
 */
int CDECL MSVCRT__stat64i32(const char* path, struct MSVCRT__stat64i32 *buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT_stat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
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

  plen = strlenW(path);
  while (plen && path[plen-1]==' ')
    plen--;

  if(plen && (plen<2 || path[plen-2]!=':') &&
          (path[plen-1]==':' || path[plen-1]=='\\' || path[plen-1]=='/'))
  {
    *MSVCRT__errno() = MSVCRT_ENOENT;
    return -1;
  }

  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed (%d)\n",GetLastError());
      *MSVCRT__errno() = MSVCRT_ENOENT;
      return -1;
  }

  memset(buf,0,sizeof(struct MSVCRT__stat64));

  /* FIXME: rdev isn't drive num, despite what the docs says-what is it? */
  if (MSVCRT_iswalpha(*path))
    buf->st_dev = buf->st_rdev = toupperW(*path - 'A'); /* drive num */
  else
    buf->st_dev = buf->st_rdev = MSVCRT__getdrive() - 1;

  /* Dir, or regular file? */
  if (hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
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
 *  _wstat32 (MSVCR100.@)
 */
int CDECL MSVCRT__wstat32(const MSVCRT_wchar_t *path, struct MSVCRT__stat32 *buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT__wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *  _wstat32i64 (MSVCR100.@)
 */
int CDECL MSVCRT__wstat32i64(const MSVCRT_wchar_t *path, struct MSVCRT__stat32i64 *buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT__wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat32i64(&buf64, buf);
    return ret;
}

/*********************************************************************
 * _wstat64i32 (MSVCR100.@)
 */
int CDECL MSVCRT__wstat64i32(const MSVCRT_wchar_t *path, struct MSVCRT__stat64i32 *buf)
{
    int ret;
    struct MSVCRT__stat64 buf64;

    ret = MSVCRT__wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_tell (MSVCRT.@)
 */
MSVCRT_long CDECL MSVCRT__tell(int fd)
{
  return MSVCRT__lseek(fd, 0, MSVCRT_SEEK_CUR);
}

/*********************************************************************
 *		_telli64 (MSVCRT.@)
 */
__int64 CDECL _telli64(int fd)
{
  return MSVCRT__lseeki64(fd, 0, MSVCRT_SEEK_CUR);
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
    ioinfo *info = get_ioinfo(fd);
    HANDLE hand = info->handle;

    /* Don't trace small writes, it gets *very* annoying */
#if 0
    if (count > 32)
        TRACE(":fd (%d) handle (%d) buf (%p) len (%d)\n",fd,hand,buf,count);
#endif
    if (hand == INVALID_HANDLE_VALUE)
    {
        *MSVCRT__errno() = MSVCRT_EBADF;
        release_ioinfo(info);
        return -1;
    }

    if (((info->exflag&EF_UTF8) || (info->exflag&EF_UTF16)) && count&1)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        release_ioinfo(info);
        return -1;
    }

    /* If appending, go to EOF */
    if (info->wxflag & WX_APPEND)
        MSVCRT__lseek(fd, 0, FILE_END);

    if (!(info->wxflag & WX_TEXT))
    {
        if (WriteFile(hand, buf, count, &num_written, NULL)
                &&  (num_written == count))
        {
            release_ioinfo(info);
            return num_written;
        }
        TRACE("WriteFile (fd %d, hand %p) failed-last error (%d)\n", fd,
                hand, GetLastError());
        *MSVCRT__errno() = MSVCRT_ENOSPC;
    }
    else
    {
        unsigned int i, j, nr_lf, size;
        char *p = NULL;
        const char *q;
        const char *s = buf, *buf_start = buf;

        if (!(info->exflag & (EF_UTF8|EF_UTF16)))
        {
            /* find number of \n */
            for (nr_lf=0, i=0; i<count; i++)
                if (s[i] == '\n')
                    nr_lf++;
            if (nr_lf)
            {
                size = count+nr_lf;
                if ((q = p = MSVCRT_malloc(size)))
                {
                    for (s = buf, i = 0, j = 0; i < count; i++)
                    {
                        if (s[i] == '\n')
                            p[j++] = '\r';
                        p[j++] = s[i];
                    }
                }
                else
                {
                    FIXME("Malloc failed\n");
                    nr_lf = 0;
                    size = count;
                    q = buf;
                }
            }
            else
            {
                size = count;
                q = buf;
            }
        }
        else if (info->exflag & EF_UTF16)
        {
            for (nr_lf=0, i=0; i<count; i+=2)
                if (s[i]=='\n' && s[i+1]==0)
                    nr_lf += 2;
            if (nr_lf)
            {
                size = count+nr_lf;
                if ((q = p = MSVCRT_malloc(size)))
                {
                    for (s=buf, i=0, j=0; i<count; i++)
                    {
                        if (s[i]=='\n' && s[i+1]==0)
                        {
                            p[j++] = '\r';
                            p[j++] = 0;
                        }
                        p[j++] = s[i++];
                        p[j++] = s[i];
                    }
                }
                else
                {
                    FIXME("Malloc failed\n");
                    nr_lf = 0;
                    size = count;
                    q = buf;
                }
            }
            else
            {
                size = count;
                q = buf;
            }
        }
        else
        {
            DWORD conv_len;

            for(nr_lf=0, i=0; i<count; i+=2)
                if (s[i]=='\n' && s[i+1]==0)
                    nr_lf++;

            conv_len = WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)buf, count/2, NULL, 0, NULL, NULL);
            if(!conv_len) {
                msvcrt_set_errno(GetLastError());
                MSVCRT_free(p);
                release_ioinfo(info);
                return -1;
            }

            size = conv_len+nr_lf;
            if((p = MSVCRT_malloc(count+nr_lf*2+size)))
            {
                for (s=buf, i=0, j=0; i<count; i++)
                {
                    if (s[i]=='\n' && s[i+1]==0)
                    {
                        p[j++] = '\r';
                        p[j++] = 0;
                    }
                    p[j++] = s[i++];
                    p[j++] = s[i];
                }
                q = p+count+nr_lf*2;
                WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)p, count/2+nr_lf,
                        p+count+nr_lf*2, conv_len+nr_lf, NULL, NULL);
            }
            else
            {
                FIXME("Malloc failed\n");
                nr_lf = 0;
                size = count;
                q = buf;
            }
        }

        if (!WriteFile(hand, q, size, &num_written, NULL))
            num_written = -1;
        release_ioinfo(info);
        if(p)
            MSVCRT_free(p);
        if (num_written != size)
        {
            TRACE("WriteFile (fd %d, hand %p) failed-last error (%d), num_written %d\n",
                    fd, hand, GetLastError(), num_written);
            *MSVCRT__errno() = MSVCRT_ENOSPC;
            return s - buf_start;
        }
        return count;
    }

    release_ioinfo(info);
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
  int ret;

  MSVCRT__lock_file(file);
  ret = MSVCRT__fclose_nolock(file);
  MSVCRT__unlock_file(file);

  return ret;
}

/*********************************************************************
 *		_fclose_nolock (MSVCRT.@)
 */
int CDECL MSVCRT__fclose_nolock(MSVCRT_FILE* file)
{
  int r, flag;

  flag = file->_flag;
  MSVCRT_free(file->_tmpfname);
  file->_tmpfname = NULL;
  /* flush stdio buffers */
  if(file->_flag & MSVCRT__IOWRT)
      MSVCRT__fflush_nolock(file);
  if(file->_flag & MSVCRT__IOMYBUF)
      MSVCRT_free(file->_base);

  r=MSVCRT__close(file->_file);
  file->_flag = 0;

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

    if(file->_flag & MSVCRT__IOSTRG)
        return MSVCRT_EOF;

    /* Allocate buffer if needed */
    if(!(file->_flag & (MSVCRT__IONBF | MSVCRT__IOMYBUF | MSVCRT__USERBUF)))
        msvcrt_alloc_buffer(file);

    if(!(file->_flag & MSVCRT__IOREAD)) {
        if(file->_flag & MSVCRT__IORW)
            file->_flag |= MSVCRT__IOREAD;
        else
            return MSVCRT_EOF;
    }

    if(!(file->_flag & (MSVCRT__IOMYBUF | MSVCRT__USERBUF))) {
        int r;
        if ((r = MSVCRT__read(file->_file,&c,1)) != 1) {
            file->_flag |= (r == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
            return MSVCRT_EOF;
        }

        return c;
    } else {
        file->_cnt = MSVCRT__read(file->_file, file->_base, file->_bufsiz);
        if(file->_cnt<=0) {
            file->_flag |= (file->_cnt == 0) ? MSVCRT__IOEOF : MSVCRT__IOERR;
            file->_cnt = 0;
            return MSVCRT_EOF;
        }

        file->_cnt--;
        file->_ptr = file->_base+1;
        c = *(unsigned char *)file->_base;
        return c;
    }
}

/*********************************************************************
 *		fgetc (MSVCRT.@)
 */
int CDECL MSVCRT_fgetc(MSVCRT_FILE* file)
{
    int ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fgetc_nolock(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fgetc_nolock (MSVCRT.@)
 */
int CDECL MSVCRT__fgetc_nolock(MSVCRT_FILE* file)
{
  unsigned char *i;
  unsigned int j;

  if (file->_cnt>0) {
    file->_cnt--;
    i = (unsigned char *)file->_ptr++;
    j = *i;
  } else
    j = MSVCRT__filbuf(file);

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

  while ((size >1) && (cc = MSVCRT__fgetc_nolock(file)) != MSVCRT_EOF && cc != '\n')
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
 */
MSVCRT_wint_t CDECL MSVCRT_fgetwc(MSVCRT_FILE* file)
{
    MSVCRT_wint_t ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fgetwc_nolock(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fgetwc_nolock (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT__fgetwc_nolock(MSVCRT_FILE* file)
{
    MSVCRT_wint_t ret;
    int ch;

    if((get_ioinfo_nolock(file->_file)->exflag & (EF_UTF8 | EF_UTF16))
            || !(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
        char *p;

        for(p=(char*)&ret; (MSVCRT_wint_t*)p<&ret+1; p++) {
            ch = MSVCRT__fgetc_nolock(file);
            if(ch == MSVCRT_EOF) {
                ret = MSVCRT_WEOF;
                break;
            }
            *p = (char)ch;
        }
    }else {
        char mbs[MSVCRT_MB_LEN_MAX];
        int len = 0;

        ch = MSVCRT__fgetc_nolock(file);
        if(ch != MSVCRT_EOF) {
            mbs[0] = (char)ch;
            if(MSVCRT_isleadbyte((unsigned char)mbs[0])) {
                ch = MSVCRT__fgetc_nolock(file);
                if(ch != MSVCRT_EOF) {
                    mbs[1] = (char)ch;
                    len = 2;
                }
            }else {
                len = 1;
            }
        }

        if(!len || MSVCRT_mbtowc(&ret, mbs, len)==-1)
            ret = MSVCRT_WEOF;
    }

    return ret;
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
    k = MSVCRT__fgetc_nolock(file);
    if (k == MSVCRT_EOF) {
      file->_flag |= MSVCRT__IOEOF;
      MSVCRT__unlock_file(file);
      return MSVCRT_EOF;
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
  MSVCRT_wint_t cc = MSVCRT_WEOF;
  MSVCRT_wchar_t * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
        file,file->_file,s,size);

  MSVCRT__lock_file(file);

  while ((size >1) && (cc = MSVCRT__fgetwc_nolock(file)) != MSVCRT_WEOF && cc != '\n')
    {
      *s++ = cc;
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
 *		_flsbuf (MSVCRT.@)
 */
int CDECL MSVCRT__flsbuf(int c, MSVCRT_FILE* file)
{
    /* Flush output buffer */
    if(!(file->_flag & (MSVCRT__IONBF | MSVCRT__IOMYBUF | MSVCRT__USERBUF))) {
        msvcrt_alloc_buffer(file);
    }

    if(!(file->_flag & MSVCRT__IOWRT)) {
        if(!(file->_flag & MSVCRT__IORW)) {
            file->_flag |= MSVCRT__IOERR;
            return MSVCRT_EOF;
        }
        file->_flag |= MSVCRT__IOWRT;
    }
    if(file->_flag & MSVCRT__IOREAD) {
        if(!(file->_flag & MSVCRT__IOEOF)) {
            file->_flag |= MSVCRT__IOERR;
            return MSVCRT_EOF;
        }
        file->_cnt = 0;
        file->_ptr = file->_base;
        file->_flag &= ~(MSVCRT__IOREAD | MSVCRT__IOEOF);
    }

    if(file->_flag & (MSVCRT__IOMYBUF | MSVCRT__USERBUF)) {
        int res = 0;

        if(file->_cnt <= 0) {
            res = msvcrt_flush_buffer(file);
            if(res)
                return res;
            file->_flag |= MSVCRT__IOWRT;
            file->_cnt=file->_bufsiz;
        }
        *file->_ptr++ = c;
        file->_cnt--;
        return c&0xff;
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
 *		fwrite (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_fwrite(const void *ptr, MSVCRT_size_t size, MSVCRT_size_t nmemb, MSVCRT_FILE* file)
{
    MSVCRT_size_t ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fwrite_nolock(ptr, size, nmemb, file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fwrite_nolock (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__fwrite_nolock(const void *ptr, MSVCRT_size_t size, MSVCRT_size_t nmemb, MSVCRT_FILE* file)
{
    MSVCRT_size_t wrcnt=size * nmemb;
    int written = 0;
    if (size == 0)
        return 0;

    while(wrcnt) {
        if(file->_cnt < 0) {
            WARN("negative file->_cnt value in %p\n", file);
            file->_flag |= MSVCRT__IOERR;
            break;
        } else if(file->_cnt) {
            int pcnt=(file->_cnt>wrcnt)? wrcnt: file->_cnt;
            memcpy(file->_ptr, ptr, pcnt);
            file->_cnt -= pcnt;
            file->_ptr += pcnt;
            written += pcnt;
            wrcnt -= pcnt;
            ptr = (const char*)ptr + pcnt;
        } else if((file->_flag & MSVCRT__IONBF)
                || ((file->_flag & (MSVCRT__IOMYBUF | MSVCRT__USERBUF)) && wrcnt >= file->_bufsiz)
                || (!(file->_flag & (MSVCRT__IOMYBUF | MSVCRT__USERBUF)) && wrcnt >= MSVCRT_INTERNAL_BUFSIZ)) {
            MSVCRT_size_t pcnt;
            int bufsiz;

            if(file->_flag & MSVCRT__IONBF)
                bufsiz = 1;
            else if(!(file->_flag & (MSVCRT__IOMYBUF | MSVCRT__USERBUF)))
                bufsiz = MSVCRT_INTERNAL_BUFSIZ;
            else
                bufsiz = file->_bufsiz;

            pcnt = (wrcnt / bufsiz) * bufsiz;

            if(msvcrt_flush_buffer(file) == MSVCRT_EOF)
                break;

            if(MSVCRT__write(file->_file, ptr, pcnt) <= 0) {
                file->_flag |= MSVCRT__IOERR;
                break;
            }
            written += pcnt;
            wrcnt -= pcnt;
            ptr = (const char*)ptr + pcnt;
        } else {
            if(MSVCRT__flsbuf(*(const char*)ptr, file) == MSVCRT_EOF)
                break;
            written++;
            wrcnt--;
            ptr = (const char*)ptr + 1;
        }
    }

    return written / size;
}

/*********************************************************************
 *		fputwc (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT_fputwc(MSVCRT_wint_t wc, MSVCRT_FILE* file)
{
    MSVCRT_wint_t ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fputwc_nolock(wc, file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fputwc_nolock (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT__fputwc_nolock(MSVCRT_wint_t wc, MSVCRT_FILE* file)
{
    MSVCRT_wchar_t mwc=wc;
    ioinfo *fdinfo;
    MSVCRT_wint_t ret;

    fdinfo = get_ioinfo_nolock(file->_file);

    if((fdinfo->wxflag&WX_TEXT) && !(fdinfo->exflag&(EF_UTF8|EF_UTF16))) {
        char buf[MSVCRT_MB_LEN_MAX];
        int char_len;

        char_len = MSVCRT_wctomb(buf, mwc);
        if(char_len!=-1 && MSVCRT__fwrite_nolock(buf, char_len, 1, file)==1)
            ret = wc;
        else
            ret = MSVCRT_WEOF;
    }else if(MSVCRT__fwrite_nolock(&mwc, sizeof(mwc), 1, file) == 1) {
        ret = wc;
    }else {
        ret = MSVCRT_WEOF;
    }

    return ret;
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
 *		fputc (MSVCRT.@)
 */
int CDECL MSVCRT_fputc(int c, MSVCRT_FILE* file)
{
    int ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fputc_nolock(c, file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fputc_nolock (MSVCRT.@)
 */
int CDECL MSVCRT__fputc_nolock(int c, MSVCRT_FILE* file)
{
  int res;

  if(file->_cnt>0) {
    *file->_ptr++=c;
    file->_cnt--;
    if (c == '\n')
    {
      res = msvcrt_flush_buffer(file);
      return res ? res : c;
    }
    else {
      return c & 0xff;
    }
  } else {
    res = MSVCRT__flsbuf(c, file);
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
    MSVCRT_size_t ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fread_nolock(ptr, size, nmemb, file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_fread_nolock (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__fread_nolock(void *ptr, MSVCRT_size_t size, MSVCRT_size_t nmemb, MSVCRT_FILE* file)
{
  MSVCRT_size_t rcnt=size * nmemb;
  MSVCRT_size_t read=0;
  MSVCRT_size_t pread=0;

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
	} else {
        return 0;
    }
  }

  if(rcnt>0 && !(file->_flag & (MSVCRT__IONBF | MSVCRT__IOMYBUF | MSVCRT__USERBUF)))
      msvcrt_alloc_buffer(file);

  while(rcnt>0)
  {
    int i;
    if (!file->_cnt && rcnt<MSVCRT_BUFSIZ && (file->_flag & (MSVCRT__IOMYBUF | MSVCRT__USERBUF))) {
      file->_cnt = MSVCRT__read(file->_file, file->_base, file->_bufsiz);
      file->_ptr = file->_base;
      i = (file->_cnt<rcnt) ? file->_cnt : rcnt;
      /* If the buffer fill reaches eof but fread wouldn't, clear eof. */
      if (i > 0 && i < file->_cnt) {
        get_ioinfo_nolock(file->_file)->wxflag &= ~WX_ATEOF;
        file->_flag &= ~MSVCRT__IOEOF;
      }
      if (i > 0) {
        memcpy(ptr, file->_ptr, i);
        file->_cnt -= i;
        file->_ptr += i;
      }
    } else if (rcnt > INT_MAX) {
      i = MSVCRT__read(file->_file, ptr, INT_MAX);
    } else if (rcnt < MSVCRT_BUFSIZ) {
      i = MSVCRT__read(file->_file, ptr, rcnt);
    } else {
      i = MSVCRT__read(file->_file, ptr, rcnt - MSVCRT_BUFSIZ/2);
    }
    pread += i;
    rcnt -= i;
    ptr = (char *)ptr+i;
    /* expose feof condition in the flags
     * MFC tests file->_flag for feof, and doesn't call feof())
     */
    if (get_ioinfo_nolock(file->_file)->wxflag & WX_ATEOF)
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
  return read / size;
}


/*********************************************************************
 *		fread_s (MSVCR80.@)
 */
MSVCRT_size_t CDECL MSVCRT_fread_s(void *buf, MSVCRT_size_t buf_size, MSVCRT_size_t elem_size,
        MSVCRT_size_t count, MSVCRT_FILE *stream)
{
    MSVCRT_size_t ret;

    if(!MSVCRT_CHECK_PMT(stream != NULL)) {
        if(buf && buf_size)
            memset(buf, 0, buf_size);
        return 0;
    }
    if(!elem_size || !count) return 0;

    MSVCRT__lock_file(stream);
    ret = MSVCRT__fread_nolock_s(buf, buf_size, elem_size, count, stream);
    MSVCRT__unlock_file(stream);

    return ret;
}

/*********************************************************************
 *		_fread_nolock_s (MSVCR80.@)
 */
MSVCRT_size_t CDECL MSVCRT__fread_nolock_s(void *buf, MSVCRT_size_t buf_size, MSVCRT_size_t elem_size,
        MSVCRT_size_t count, MSVCRT_FILE *stream)
{
    size_t bytes_left, buf_pos;

    TRACE("(%p %lu %lu %lu %p)\n", buf, buf_size, elem_size, count, stream);

    if(!MSVCRT_CHECK_PMT(stream != NULL)) {
        if(buf && buf_size)
            memset(buf, 0, buf_size);
        return 0;
    }
    if(!elem_size || !count) return 0;
    if(!MSVCRT_CHECK_PMT(buf != NULL)) return 0;
    if(!MSVCRT_CHECK_PMT(MSVCRT_SIZE_MAX/count >= elem_size)) return 0;

    bytes_left = elem_size*count;
    buf_pos = 0;
    while(bytes_left) {
        if(stream->_cnt > 0) {
            size_t size = bytes_left<stream->_cnt ? bytes_left : stream->_cnt;

            if(!MSVCRT_CHECK_PMT_ERR(size <= buf_size-buf_pos, MSVCRT_ERANGE)) {
                memset(buf, 0, buf_size);
                return 0;
            }

            MSVCRT__fread_nolock((char*)buf+buf_pos, 1, size, stream);
            buf_pos += size;
            bytes_left -= size;
        }else {
            int c = MSVCRT__filbuf(stream);

            if(c == MSVCRT_EOF)
                break;

            if(!MSVCRT_CHECK_PMT_ERR(buf_size != buf_pos, MSVCRT_ERANGE)) {
                memset(buf, 0, buf_size);
                return 0;
            }

            ((char*)buf)[buf_pos++] = c;
            bytes_left--;
        }
    }

    return buf_pos/elem_size;
}

/*********************************************************************
 *		_wfreopen (MSVCRT.@)
 *
 */
MSVCRT_FILE* CDECL MSVCRT__wfreopen(const MSVCRT_wchar_t *path, const MSVCRT_wchar_t *mode, MSVCRT_FILE* file)
{
    int open_flags, stream_flags, fd;

    TRACE(":path (%s) mode (%s) file (%p) fd (%d)\n", debugstr_w(path), debugstr_w(mode), file, file ? file->_file : -1);

    LOCK_FILES();
    if (!file || ((fd = file->_file) < 0))
        file = NULL;
    else
    {
        MSVCRT_fclose(file);
        if (msvcrt_get_flags(mode, &open_flags, &stream_flags) == -1)
            file = NULL;
        else if((fd = MSVCRT__wopen(path, open_flags, MSVCRT__S_IREAD | MSVCRT__S_IWRITE)) < 0)
            file = NULL;
        else if(msvcrt_init_fp(file, fd, stream_flags) == -1)
        {
            file->_flag = 0;
            file = NULL;
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
  msvcrt_flush_buffer(file);

  /* Reset direction of i/o */
  if(file->_flag & MSVCRT__IORW) {
        file->_flag &= ~(MSVCRT__IOREAD|MSVCRT__IOWRT);
  }

  ret = (MSVCRT__lseeki64(file->_file,*pos,MSVCRT_SEEK_SET) == -1) ? -1 : 0;
  MSVCRT__unlock_file(file);
  return ret;
}

/*********************************************************************
 *		_ftelli64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__ftelli64(MSVCRT_FILE* file)
{
    __int64 ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__ftelli64_nolock(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_ftelli64_nolock (MSVCRT.@)
 */
__int64 CDECL MSVCRT__ftelli64_nolock(MSVCRT_FILE* file)
{
    __int64 pos;

    pos = _telli64(file->_file);
    if(pos == -1)
        return -1;
    if(file->_flag & (MSVCRT__IOMYBUF | MSVCRT__USERBUF))  {
        if(file->_flag & MSVCRT__IOWRT) {
            pos += file->_ptr - file->_base;

            if(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT) {
                char *p;

                for(p=file->_base; p<file->_ptr; p++)
                    if(*p == '\n')
                        pos++;
            }
        } else if(!file->_cnt) { /* nothing to do */
        } else if(MSVCRT__lseeki64(file->_file, 0, MSVCRT_SEEK_END)==pos) {
            int i;

            pos -= file->_cnt;
            if(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT) {
                for(i=0; i<file->_cnt; i++)
                    if(file->_ptr[i] == '\n')
                        pos--;
            }
        } else {
            char *p;

            if(MSVCRT__lseeki64(file->_file, pos, MSVCRT_SEEK_SET) != pos)
                return -1;

            pos -= file->_bufsiz;
            pos += file->_ptr - file->_base;

            if(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT) {
                if(get_ioinfo_nolock(file->_file)->wxflag & WX_READNL)
                    pos--;

                for(p=file->_base; p<file->_ptr; p++)
                    if(*p == '\n')
                        pos++;
            }
        }
    }

    return pos;
}

/*********************************************************************
 *		ftell (MSVCRT.@)
 */
LONG CDECL MSVCRT_ftell(MSVCRT_FILE* file)
{
  return MSVCRT__ftelli64(file);
}

/*********************************************************************
 *		_ftell_nolock (MSVCRT.@)
 */
LONG CDECL MSVCRT__ftell_nolock(MSVCRT_FILE* file)
{
  return MSVCRT__ftelli64_nolock(file);
}

/*********************************************************************
 *		fgetpos (MSVCRT.@)
 */
int CDECL MSVCRT_fgetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
    *pos = MSVCRT__ftelli64(file);
    if(*pos == -1)
        return -1;
    return 0;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int CDECL MSVCRT_fputs(const char *s, MSVCRT_FILE* file)
{
    MSVCRT_size_t len = strlen(s);
    int ret;

    MSVCRT__lock_file(file);
    ret = MSVCRT__fwrite_nolock(s, sizeof(*s), len, file) == len ? 0 : MSVCRT_EOF;
    MSVCRT__unlock_file(file);
    return ret;
}

/*********************************************************************
 *		fputws (MSVCRT.@)
 */
int CDECL MSVCRT_fputws(const MSVCRT_wchar_t *s, MSVCRT_FILE* file)
{
    MSVCRT_size_t i, len = strlenW(s);
    BOOL tmp_buf;
    int ret;

    MSVCRT__lock_file(file);
    if (!(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
        ret = MSVCRT__fwrite_nolock(s,sizeof(*s),len,file) == len ? 0 : MSVCRT_EOF;
        MSVCRT__unlock_file(file);
        return ret;
    }

    tmp_buf = add_std_buffer(file);
    for (i=0; i<len; i++) {
        if(MSVCRT__fputwc_nolock(s[i], file) == MSVCRT_WEOF) {
            if(tmp_buf) remove_std_buffer(file);
            MSVCRT__unlock_file(file);
            return MSVCRT_WEOF;
        }
    }

    if(tmp_buf) remove_std_buffer(file);
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
  for(cc = MSVCRT__fgetc_nolock(MSVCRT_stdin); cc != MSVCRT_EOF && cc != '\n';
      cc = MSVCRT__fgetc_nolock(MSVCRT_stdin))
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
    for (cc = MSVCRT__fgetwc_nolock(MSVCRT_stdin); cc != MSVCRT_WEOF && cc != '\n';
         cc = MSVCRT__fgetwc_nolock(MSVCRT_stdin))
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
 *		puts (MSVCRT.@)
 */
int CDECL MSVCRT_puts(const char *s)
{
    MSVCRT_size_t len = strlen(s);
    int ret;

    MSVCRT__lock_file(MSVCRT_stdout);
    if(MSVCRT__fwrite_nolock(s, sizeof(*s), len, MSVCRT_stdout) != len) {
        MSVCRT__unlock_file(MSVCRT_stdout);
        return MSVCRT_EOF;
    }

    ret = MSVCRT__fwrite_nolock("\n",1,1,MSVCRT_stdout) == 1 ? 0 : MSVCRT_EOF;
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
    if(MSVCRT__fwrite_nolock(s, sizeof(*s), len, MSVCRT_stdout) != len) {
        MSVCRT__unlock_file(MSVCRT_stdout);
        return MSVCRT_EOF;
    }

    ret = MSVCRT__fwrite_nolock(&nl,sizeof(nl),1,MSVCRT_stdout) == 1 ? 0 : MSVCRT_EOF;
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
    if(!MSVCRT_CHECK_PMT(file != NULL)) return -1;
    if(!MSVCRT_CHECK_PMT(mode==MSVCRT__IONBF || mode==MSVCRT__IOFBF || mode==MSVCRT__IOLBF)) return -1;
    if(!MSVCRT_CHECK_PMT(mode==MSVCRT__IONBF || (size>=2 && size<=INT_MAX))) return -1;

    MSVCRT__lock_file(file);

    MSVCRT__fflush_nolock(file);
    if(file->_flag & MSVCRT__IOMYBUF)
        MSVCRT_free(file->_base);
    file->_flag &= ~(MSVCRT__IONBF | MSVCRT__IOMYBUF | MSVCRT__USERBUF);
    file->_cnt = 0;

    if(mode == MSVCRT__IONBF) {
        file->_flag |= MSVCRT__IONBF;
        file->_base = file->_ptr = (char*)&file->_charbuf;
        file->_bufsiz = 2;
    }else if(buf) {
        file->_base = file->_ptr = buf;
        file->_flag |= MSVCRT__USERBUF;
        file->_bufsiz = size;
    }else {
        file->_base = file->_ptr = MSVCRT_malloc(size);
        if(!file->_base) {
            file->_bufsiz = 0;
            MSVCRT__unlock_file(file);
            return -1;
        }

        file->_flag |= MSVCRT__IOMYBUF;
        file->_bufsiz = size;
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

static int tmpnam_helper(char *s, MSVCRT_size_t size, int *tmpnam_unique, int tmp_max)
{
    char tmpstr[8];
    char *p = s;
    int digits;

    if (!MSVCRT_CHECK_PMT(s != NULL)) return MSVCRT_EINVAL;

    if (size < 3) {
        if (size) *s = 0;
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }
    *p++ = '\\';
    *p++ = 's';
    size -= 2;
    digits = msvcrt_int_to_base32(GetCurrentProcessId(), tmpstr);
    if (digits+1 > size) {
        *s = 0;
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }
    memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
    p += digits;
    *p++ = '.';
    size -= digits+1;

    while(1) {
        while ((digits = *tmpnam_unique)+1 < tmp_max) {
            if (InterlockedCompareExchange(tmpnam_unique, digits+1, digits) == digits)
                break;
        }

        digits = msvcrt_int_to_base32(digits, tmpstr);
        if (digits+1 > size) {
            *s = 0;
            *MSVCRT__errno() = MSVCRT_ERANGE;
            return MSVCRT_ERANGE;
        }
        memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
        p[digits] = 0;

        if (GetFileAttributesA(s) == INVALID_FILE_ATTRIBUTES &&
                GetLastError() == ERROR_FILE_NOT_FOUND)
            break;
    }
    return 0;
}

int CDECL MSVCRT_tmpnam_s(char *s, MSVCRT_size_t size)
{
    return tmpnam_helper(s, size, &tmpnam_s_unique, MSVCRT_TMP_MAX_S);
}

/*********************************************************************
 *		tmpnam (MSVCRT.@)
 */
char * CDECL MSVCRT_tmpnam(char *s)
{
  if (!s) {
    thread_data_t *data = msvcrt_get_thread_data();

    if(!data->tmpnam_buffer)
      data->tmpnam_buffer = MSVCRT_malloc(MAX_PATH);

    s = data->tmpnam_buffer;
  }

  return tmpnam_helper(s, -1, &tmpnam_unique, MSVCRT_TMP_MAX) ? NULL : s;
}

static int wtmpnam_helper(MSVCRT_wchar_t *s, MSVCRT_size_t size, int *tmpnam_unique, int tmp_max)
{
    MSVCRT_wchar_t tmpstr[8];
    MSVCRT_wchar_t *p = s;
    int digits;

    if (!MSVCRT_CHECK_PMT(s != NULL)) return MSVCRT_EINVAL;

    if (size < 3) {
        if (size) *s = 0;
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }
    *p++ = '\\';
    *p++ = 's';
    size -= 2;
    digits = msvcrt_int_to_base32_w(GetCurrentProcessId(), tmpstr);
    if (digits+1 > size) {
        *s = 0;
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }
    memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
    p += digits;
    *p++ = '.';
    size -= digits+1;

    while(1) {
        while ((digits = *tmpnam_unique)+1 < tmp_max) {
            if (InterlockedCompareExchange(tmpnam_unique, digits+1, digits) == digits)
                break;
        }

        digits = msvcrt_int_to_base32_w(digits, tmpstr);
        if (digits+1 > size) {
            *s = 0;
            *MSVCRT__errno() = MSVCRT_ERANGE;
            return MSVCRT_ERANGE;
        }
        memcpy(p, tmpstr, digits*sizeof(tmpstr[0]));
        p[digits] = 0;

        if (GetFileAttributesW(s) == INVALID_FILE_ATTRIBUTES &&
                GetLastError() == ERROR_FILE_NOT_FOUND)
            break;
    }
    return 0;
}

/*********************************************************************
 *              _wtmpnam_s (MSVCRT.@)
 */
int CDECL MSVCRT__wtmpnam_s(MSVCRT_wchar_t *s, MSVCRT_size_t size)
{
    return wtmpnam_helper(s, size, &tmpnam_s_unique, MSVCRT_TMP_MAX_S);
}

/*********************************************************************
 *              _wtmpnam (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT__wtmpnam(MSVCRT_wchar_t *s)
{
    if (!s) {
        thread_data_t *data = msvcrt_get_thread_data();

        if(!data->wtmpnam_buffer)
            data->wtmpnam_buffer = MSVCRT_malloc(sizeof(MSVCRT_wchar_t[MAX_PATH]));

        s = data->wtmpnam_buffer;
    }

    return wtmpnam_helper(s, -1, &tmpnam_unique, MSVCRT_TMP_MAX) ? NULL : s;
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
  fd = MSVCRT__open(filename, MSVCRT__O_CREAT | MSVCRT__O_BINARY | MSVCRT__O_RDWR | MSVCRT__O_TEMPORARY,
          MSVCRT__S_IREAD | MSVCRT__S_IWRITE);
  if (fd != -1 && (file = msvcrt_alloc_fp()))
  {
    if (msvcrt_init_fp(file, fd, MSVCRT__IORW) == -1)
    {
        file->_flag = 0;
        file = NULL;
    }
    else file->_tmpfname = MSVCRT__strdup(filename);
  }

  if(fd != -1 && !file)
      MSVCRT__close(fd);
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
    int i, ret;

    MSVCRT__lock_file(file);

    if(!(get_ioinfo_nolock(((MSVCRT_FILE*)file)->_file)->wxflag & WX_TEXT)) {
        ret = MSVCRT__fwrite_nolock(str, sizeof(MSVCRT_wchar_t), len, file);
        MSVCRT__unlock_file(file);
        return ret;
    }

    for(i=0; i<len; i++) {
        if(MSVCRT__fputwc_nolock(str[i], file) == MSVCRT_WEOF) {
            MSVCRT__unlock_file(file);
            return -1;
        }
    }

    MSVCRT__unlock_file(file);
    return len;
}

/*********************************************************************
 *		vfprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vfprintf(MSVCRT_FILE* file, const char *format, __ms_va_list valist)
{
    BOOL tmp_buf;
    int ret;

    MSVCRT__lock_file(file);
    tmp_buf = add_std_buffer(file);
    ret = pf_printf_a(puts_clbk_file_a, file, format, NULL, 0, arg_clbk_valist, NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		vfprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vfprintf_s(MSVCRT_FILE* file, const char *format, __ms_va_list valist)
{
    BOOL tmp_buf;
    int ret;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return -1;

    MSVCRT__lock_file(file);
    tmp_buf = add_std_buffer(file);
    ret = pf_printf_a(puts_clbk_file_a, file, format, NULL,
            MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER, arg_clbk_valist, NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		vfwprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vfwprintf(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    BOOL tmp_buf;
    int ret;

    MSVCRT__lock_file(file);
    tmp_buf = add_std_buffer(file);
    ret = pf_printf_w(puts_clbk_file_w, file, format, NULL, 0, arg_clbk_valist, NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		vfwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vfwprintf_s(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    BOOL tmp_buf;
    int ret;

    if (!MSVCRT_CHECK_PMT( file != NULL )) return -1;

    MSVCRT__lock_file(file);
    tmp_buf = add_std_buffer(file);
    ret = pf_printf_w(puts_clbk_file_w, file, format, NULL,
            MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER, arg_clbk_valist, NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *              __stdio_common_vfprintf (MSVCRT.@)
 */
int CDECL MSVCRT__stdio_common_vfprintf(unsigned __int64 options, MSVCRT_FILE *file, const char *format,
                                        MSVCRT__locale_t locale, __ms_va_list valist)
{
    BOOL tmp_buf;
    int ret;

    if (!MSVCRT_CHECK_PMT( file != NULL )) return -1;

    MSVCRT__lock_file(file);
    tmp_buf = add_std_buffer(file);

    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    ret = pf_printf_a(puts_clbk_file_a, file, format, locale,
            options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		__stdio_common_vfwprintf (MSVCRT.@)
 */
int CDECL MSVCRT__stdio_common_vfwprintf(unsigned __int64 options, MSVCRT_FILE *file, const MSVCRT_wchar_t *format,
                                         MSVCRT__locale_t locale, __ms_va_list valist)
{
    BOOL tmp_buf;
    int ret;

    if (!MSVCRT_CHECK_PMT( file != NULL )) return -1;
    if (!MSVCRT_CHECK_PMT( format != NULL )) return -1;

    MSVCRT__lock_file(file);
    tmp_buf = add_std_buffer(file);

    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));

    ret = pf_printf_w(puts_clbk_file_w, file, format, locale,
            options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);

    if(tmp_buf) remove_std_buffer(file);
    MSVCRT__unlock_file(file);

    return ret;
}


/*********************************************************************
 *              _vfwprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT__vfwprintf_l(MSVCRT_FILE* file, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist)
{
    BOOL tmp_buf;
    int ret;

    if (!MSVCRT_CHECK_PMT( file != NULL )) return -1;

    MSVCRT__lock_file(file);
    tmp_buf = add_std_buffer(file);
    ret = pf_printf_w(puts_clbk_file_w, file, format, locale, 0, arg_clbk_valist, NULL, &valist);
    if(tmp_buf) remove_std_buffer(file);
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
 *              _fwprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT__fwprintf_l(MSVCRT_FILE* file, const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, ...)
{
    __ms_va_list valist;
    int res;
    __ms_va_start(valist, locale);
    res = MSVCRT__vfwprintf_l(file, format, locale, valist);
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
    int ret;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_EOF;

    MSVCRT__lock_file(file);
    ret = MSVCRT__ungetc_nolock(c, file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *		_ungetc_nolock (MSVCRT.@)
 */
int CDECL MSVCRT__ungetc_nolock(int c, MSVCRT_FILE * file)
{
    if(!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_EOF;

    if (c == MSVCRT_EOF || !(file->_flag&MSVCRT__IOREAD ||
                (file->_flag&MSVCRT__IORW && !(file->_flag&MSVCRT__IOWRT))))
        return MSVCRT_EOF;

    if((!(file->_flag & (MSVCRT__IONBF | MSVCRT__IOMYBUF | MSVCRT__USERBUF))
                && msvcrt_alloc_buffer(file))
            || (!file->_cnt && file->_ptr==file->_base))
        file->_ptr++;

    if(file->_ptr>file->_base) {
        file->_ptr--;
        if(file->_flag & MSVCRT__IOSTRG) {
            if(*file->_ptr != c) {
                file->_ptr++;
                return MSVCRT_EOF;
            }
        }else {
            *file->_ptr = c;
        }
        file->_cnt++;
        file->_flag &= ~(MSVCRT__IOERR | MSVCRT__IOEOF);
        file->_flag |= MSVCRT__IOREAD;
        return c;
    }

    return MSVCRT_EOF;
}

/*********************************************************************
 *              ungetwc (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT_ungetwc(MSVCRT_wint_t wc, MSVCRT_FILE * file)
{
    MSVCRT_wint_t ret;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_WEOF;

    MSVCRT__lock_file(file);
    ret = MSVCRT__ungetwc_nolock(wc, file);
    MSVCRT__unlock_file(file);

    return ret;
}

/*********************************************************************
 *              _ungetwc_nolock (MSVCRT.@)
 */
MSVCRT_wint_t CDECL MSVCRT__ungetwc_nolock(MSVCRT_wint_t wc, MSVCRT_FILE * file)
{
    MSVCRT_wchar_t mwc = wc;

    if(!MSVCRT_CHECK_PMT(file != NULL)) return MSVCRT_WEOF;
    if (wc == MSVCRT_WEOF)
        return MSVCRT_WEOF;

    if((get_ioinfo_nolock(file->_file)->exflag & (EF_UTF8 | EF_UTF16))
            || !(get_ioinfo_nolock(file->_file)->wxflag & WX_TEXT)) {
        unsigned char * pp = (unsigned char *)&mwc;
        int i;

        for(i=sizeof(MSVCRT_wchar_t)-1;i>=0;i--) {
            if(pp[i] != MSVCRT__ungetc_nolock(pp[i],file))
                return MSVCRT_WEOF;
        }
    }else {
        char mbs[MSVCRT_MB_LEN_MAX];
        int len;

        len = MSVCRT_wctomb(mbs, mwc);
        if(len == -1)
            return MSVCRT_WEOF;

        for(len--; len>=0; len--) {
            if(mbs[len] != MSVCRT__ungetc_nolock(mbs[len], file))
                return MSVCRT_WEOF;
        }
    }

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

/*********************************************************************
 *		_get_stream_buffer_pointers (UCRTBASE.@)
 */
int CDECL MSVCRT__get_stream_buffer_pointers(MSVCRT_FILE *file, char*** base,
                                             char*** ptr, int** count)
{
    if (base)
        *base = &file->_base;
    if (ptr)
        *ptr = &file->_ptr;
    if (count)
        *count = &file->_cnt;
    return 0;
}
