/*
 * msvcrt.dll file functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 */
#include <time.h>
#include "ntddk.h"
#include "msvcrt.h"
#include "ms_errno.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

/* stat() mode bits */
#define _S_IFMT                  0170000
#define _S_IFREG                 0100000
#define _S_IFDIR                 0040000
#define _S_IFCHR                 0020000
#define _S_IFIFO                 0010000
#define _S_IREAD                 0000400
#define _S_IWRITE                0000200
#define _S_IEXEC                 0000100

/* for stat mode, permissions apply to all,owner and group */
#define MSVCRT_S_IREAD  (_S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6))
#define MSVCRT_S_IWRITE (_S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6))
#define MSVCRT_S_IEXEC  (_S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6))

/* _open modes */
#define _O_RDONLY 0x0000
#define _O_WRONLY 0x0001
#define _O_RDWR   0x0002
#define _O_APPEND 0x0008
#define _O_CREAT  0x0100
#define _O_TRUNC  0x0200
#define _O_EXCL   0x0400
#define _O_TEXT   0x4000
#define _O_BINARY 0x8000
#define _O_TEMPORARY 0x0040 /* Will be closed and deleted on exit */

/* _access() bit flags FIXME: incomplete */
#define W_OK      2

typedef struct _crtfile
{
  char* _ptr;
  int   _cnt;
  char* _base;
  int   _flag;
  int   _file; /* fd */
  int   _charbuf;
  int   _bufsiz;
  char *_tmpfname;
} MSVCRT_FILE;

/* file._flag flags */
#define _IOREAD   0x0001
#define _IOWRT    0x0002
#define _IOEOF    0x0010
#define _IOERR    0x0020
#define _IORW     0x0080
#define _IOAPPEND 0x0200

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define MSVCRT_stdin  (&MSVCRT__iob[0])
#define MSVCRT_stdout (&MSVCRT__iob[1])
#define MSVCRT_stderr (&MSVCRT__iob[2])

struct _stat
{
    unsigned short st_dev;
    unsigned short st_ino;
    unsigned short st_mode;
    short  st_nlink;
    short  st_uid;
    short  st_gid;
    unsigned int   st_rdev;
    int    st_size;
    int    st_atime;
    int    st_mtime;
    int    st_ctime;
};

typedef long MSVCRT_fpos_t;

struct _utimbuf
{
  time_t actime;
  time_t modtime;
};

/* FIXME: Make this dynamic */
#define MSVCRT_MAX_FILES 257

HANDLE MSVCRT_handles[MSVCRT_MAX_FILES];
MSVCRT_FILE* MSVCRT_files[MSVCRT_MAX_FILES];
int  MSVCRT_flags[MSVCRT_MAX_FILES];
char *MSVCRT_tempfiles[MSVCRT_MAX_FILES];
MSVCRT_FILE MSVCRT__iob[3];

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

extern CRITICAL_SECTION MSVCRT_file_cs;
#define LOCK_FILES     EnterCriticalSection(&MSVCRT_file_cs)
#define UNLOCK_FILES   LeaveCriticalSection(&MSVCRT_file_cs)

time_t __cdecl MSVCRT_time(time_t *);
int __cdecl MSVCRT__getdrive(void);

/* INTERNAL: Get the HANDLE for a fd */
static HANDLE MSVCRT__fdtoh(int fd)
{
  if (fd < 0 || fd >= MSVCRT_fdend ||
      MSVCRT_handles[fd] == INVALID_HANDLE_VALUE)
  {
    WARN(":fd (%d) - no handle!\n",fd);
    SET_THREAD_VAR(doserrno,0);
    SET_THREAD_VAR(errno,MSVCRT_EBADF);
   return INVALID_HANDLE_VALUE;
  }
  return MSVCRT_handles[fd];
}

/* INTERNAL: free a file entry fd */
static void MSVCRT__free_fd(int fd)
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
static int MSVCRT__alloc_fd(HANDLE hand, int flag)
{
  int fd = MSVCRT_fdstart;

  TRACE(":handle (%d) allocating fd (%d)\n",hand,fd);
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
static MSVCRT_FILE* MSVCRT__alloc_fp(int fd)
{
  TRACE(":fd (%d) allocating FILE*\n",fd);
  if (fd < 0 || fd >= MSVCRT_fdend ||
      MSVCRT_handles[fd] == INVALID_HANDLE_VALUE)
  {
    WARN(":invalid fd %d\n",fd);
    SET_THREAD_VAR(doserrno,0);
    SET_THREAD_VAR(errno,MSVCRT_EBADF);
    return NULL;
  }
  if (!MSVCRT_files[fd])
  {
    if ((MSVCRT_files[fd] = MSVCRT_calloc(sizeof(MSVCRT_FILE),1)))
    {
      MSVCRT_files[fd]->_file = fd;
      MSVCRT_files[fd]->_flag = MSVCRT_flags[fd];
      MSVCRT_files[fd]->_flag &= ~_IOAPPEND; /* mask out, see above */
    }
  }
  TRACE(":got FILE* (%p)\n",MSVCRT_files[fd]);
  return MSVCRT_files[fd];
}


/* INTERNAL: Set up stdin, stderr and stdout */
void MSVCRT_init_io(void)
{
  int i;
  memset(MSVCRT__iob,0,3*sizeof(MSVCRT_FILE));
  MSVCRT_handles[0] = GetStdHandle(STD_INPUT_HANDLE);
  MSVCRT_flags[0] = MSVCRT__iob[0]._flag = _IOREAD;
  MSVCRT_handles[1] = GetStdHandle(STD_OUTPUT_HANDLE);
  MSVCRT_flags[1] = MSVCRT__iob[1]._flag = _IOWRT;
  MSVCRT_handles[2] = GetStdHandle(STD_ERROR_HANDLE);
  MSVCRT_flags[2] = MSVCRT__iob[2]._flag = _IOWRT;

  TRACE(":handles (%d)(%d)(%d)\n",MSVCRT_handles[0],
	MSVCRT_handles[1],MSVCRT_handles[2]);

  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    MSVCRT_files[i] = &MSVCRT__iob[i];
    MSVCRT__iob[i]._file = i;
    MSVCRT_tempfiles[i] = NULL;
  }
}

/*********************************************************************
 *		__p__iob(MSVCRT.@)
 */
MSVCRT_FILE *MSVCRT___p__iob(void)
{
 return &MSVCRT__iob[0];
}

/*********************************************************************
 *		_access (MSVCRT.@)
 */
int __cdecl MSVCRT__access(const char *filename, int mode)
{
  DWORD attr = GetFileAttributesA(filename);

  if (attr == 0xffffffff)
  {
    if (!filename)
    {
	/* FIXME: Should GetFileAttributesA() return this? */
      MSVCRT__set_errno(ERROR_INVALID_DATA);
      return -1;
    }
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & W_OK))
  {
    MSVCRT__set_errno(ERROR_ACCESS_DENIED);
    return -1;
  }
  TRACE(":file %s, mode (%d) ok\n",filename,mode);
  return 0;
}


/*********************************************************************
 *		_chmod (MSVCRT.@)
 */
int __cdecl MSVCRT__chmod(const char *path, int flags)
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
 *		_unlink (MSVCRT.@)
 */
int __cdecl MSVCRT__unlink(const char *path)
{
  TRACE("path (%s)\n",path);
  if(DeleteFileA(path))
    return 0;
  TRACE("failed-last error (%ld)\n",GetLastError());
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_close (MSVCRT.@)
 */
int __cdecl MSVCRT__close(int fd)
{
  HANDLE hand = MSVCRT__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Dont free std FILE*'s, they are not dynamic */
  if (fd > 2 && MSVCRT_files[fd])
    MSVCRT_free(MSVCRT_files[fd]);

  MSVCRT__free_fd(fd);

  if (!CloseHandle(hand))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  if (MSVCRT_tempfiles[fd])
  {
    TRACE("deleting temporary file '%s'\n",MSVCRT_tempfiles[fd]);
    MSVCRT__unlink(MSVCRT_tempfiles[fd]);
    MSVCRT_free(MSVCRT_tempfiles[fd]);
    MSVCRT_tempfiles[fd] = NULL;
  }

  TRACE(":ok\n");
  return 0;
}

/*********************************************************************
 *		_commit (MSVCRT.@)
 */
int __cdecl MSVCRT__commit(int fd)
{
  HANDLE hand = MSVCRT__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);
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
int __cdecl MSVCRT__eof(int fd)
{
  DWORD curpos,endpos;
  HANDLE hand = MSVCRT__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);

  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* If we have a FILE* for this file, the EOF flag
   * will be set by the read()/write() functions.
   */
  if (MSVCRT_files[fd])
    return MSVCRT_files[fd]->_flag & _IOEOF;

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
int __cdecl MSVCRT__fcloseall(void)
{
  int num_closed = 0, i;

  for (i = 3; i < MSVCRT_fdend; i++)
    if (MSVCRT_handles[i] != INVALID_HANDLE_VALUE)
    {
      MSVCRT__close(i);
      num_closed++;
    }

  TRACE(":closed (%d) handles\n",num_closed);
  return num_closed;
}

/*********************************************************************
 *		_lseek (MSVCRT.@)
 */
LONG __cdecl MSVCRT__lseek(int fd, LONG offset, int whence)
{
  DWORD ret;
  HANDLE hand = MSVCRT__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (whence < 0 || whence > 2)
  {
    SET_THREAD_VAR(errno,MSVCRT_EINVAL);
    return -1;
  }

  TRACE(":fd (%d) to 0x%08lx pos %s\n",
        fd,offset,(whence==SEEK_SET)?"SEEK_SET":
        (whence==SEEK_CUR)?"SEEK_CUR":
        (whence==SEEK_END)?"SEEK_END":"UNKNOWN");

  if ((ret = SetFilePointer(hand, offset, NULL, whence)) != 0xffffffff)
  {
    if (MSVCRT_files[fd])
      MSVCRT_files[fd]->_flag &= ~_IOEOF;
    /* FIXME: What if we seek _to_ EOF - is EOF set? */
    return ret;
  }
  TRACE(":error-last error (%ld)\n",GetLastError());
  if (MSVCRT_files[fd])
    switch(GetLastError())
    {
    case ERROR_NEGATIVE_SEEK:
    case ERROR_SEEK_ON_DEVICE:
      MSVCRT__set_errno(GetLastError());
      MSVCRT_files[fd]->_flag |= _IOERR;
      break;
    default:
      break;
    }
  return -1;
}

/*********************************************************************
 *		rewind (MSVCRT.@)
 */
VOID __cdecl MSVCRT_rewind(MSVCRT_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);
  MSVCRT__lseek(file->_file,0,SEEK_SET);
  file->_flag &= ~(_IOEOF | _IOERR);
}

/*********************************************************************
 *		_fdopen (MSVCRT.@)
 */
MSVCRT_FILE* __cdecl MSVCRT__fdopen(int fd, const char *mode)
{
  MSVCRT_FILE* file = MSVCRT__alloc_fp(fd);

  TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,mode,file);
  if (file)
    MSVCRT_rewind(file);

  return file;
}

/*********************************************************************
 *		_filelength (MSVCRT.@)
 */
LONG __cdecl MSVCRT__filelength(int fd)
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
 *		_fileno (MSVCRT.@)
 */
int __cdecl MSVCRT__fileno(MSVCRT_FILE* file)
{
  TRACE(":FILE* (%p) fd (%d)\n",file,file->_file);
  return file->_file;
}

/*********************************************************************
 *		_flushall (MSVCRT.@)
 */
int __cdecl MSVCRT__flushall(VOID)
{
  int num_flushed = 0, i = 3;

  while(i < MSVCRT_fdend)
    if (MSVCRT_handles[i] != INVALID_HANDLE_VALUE)
    {
      if (MSVCRT__commit(i) == -1)
	if (MSVCRT_files[i])
	  MSVCRT_files[i]->_flag |= _IOERR;
      num_flushed++;
    }

  TRACE(":flushed (%d) handles\n",num_flushed);
  return num_flushed;
}

/*********************************************************************
 *		_fstat (MSVCRT.@)
 */
int __cdecl MSVCRT__fstat(int fd, struct _stat* buf)
{
  DWORD dw;
  BY_HANDLE_FILE_INFORMATION hfi;
  HANDLE hand = MSVCRT__fdtoh(fd);

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
  memset(buf, 0, sizeof(struct _stat));
  if (!GetFileInformationByHandle(hand, &hfi))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    MSVCRT__set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }
  FIXME(":dwFileAttributes = %d, mode set to 0",hfi.dwFileAttributes);
  buf->st_nlink = hfi.nNumberOfLinks;
  buf->st_size  = hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970(&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970(&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  return 0;
}

/*********************************************************************
 *		_futime (MSVCRT.@)
 */
int __cdecl MSVCRT__futime(int fd, struct _utimbuf *t)
{
  HANDLE hand = MSVCRT__fdtoh(fd);
  FILETIME at, wt;

  if (!t)
  {
    time_t currTime;
    MSVCRT_time(&currTime);
    RtlSecondsSince1970ToTime(currTime, &at);
    memcpy(&wt, &at, sizeof(wt));
  }
  else
  {
    RtlSecondsSince1970ToTime(t->actime, &at);
    if (t->actime == t->modtime)
      memcpy(&wt, &at, sizeof(wt));
    else
      RtlSecondsSince1970ToTime(t->modtime, &wt);
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
HANDLE MSVCRT__get_osfhandle(int fd)
{
  HANDLE hand = MSVCRT__fdtoh(fd);
  HANDLE newhand = hand;
  TRACE(":fd (%d) handle (%d)\n",fd,hand);

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
  return newhand;
}

/*********************************************************************
 *		_isatty (MSVCRT.@)
 */
int __cdecl MSVCRT__isatty(int fd)
{
  HANDLE hand = MSVCRT__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return 0;

  return GetFileType(fd) == FILE_TYPE_CHAR? 1 : 0;
}

/*********************************************************************
 *		_mktemp (MSVCRT.@)
 */
char *__cdecl MSVCRT__mktemp(char *pattern)
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
 *		_open (MSVCRT.@)
 */
int __cdecl MSVCRT__open(const char *path,int flags)
{
  DWORD access = 0, creation = 0;
  int ioflag = 0, fd;
  HANDLE hand;

  TRACE(":file (%s) mode 0x%04x\n",path,flags);

  switch(flags & (_O_RDONLY | _O_WRONLY | _O_RDWR))
  {
  case _O_RDONLY:
    access |= GENERIC_READ;
    ioflag |= _IOREAD;
    break;
  case _O_WRONLY:
    access |= GENERIC_WRITE;
    ioflag |= _IOWRT;
    break;
  case _O_RDWR:
    access |= GENERIC_WRITE | GENERIC_READ;
    ioflag |= _IORW;
    break;
  }

  if (flags & _O_CREAT)
  {
    if (flags & _O_EXCL)
      creation = CREATE_NEW;
    else if (flags & _O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  }
  else  /* no _O_CREAT */
  {
    if (flags & _O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }
  if (flags & _O_APPEND)
    ioflag |= _IOAPPEND;


  flags |= _O_BINARY; /* FIXME: Default to text */

  if (flags & _O_TEXT)
  {
    /* Dont warn when writing */
    if (ioflag & GENERIC_READ)
      FIXME(":TEXT node not implemented\n");
    flags &= ~_O_TEXT;
  }

  if (flags & ~(_O_BINARY|_O_TEXT|_O_APPEND|_O_TRUNC|_O_EXCL
                |_O_CREAT|_O_RDWR|_O_TEMPORARY))
    TRACE(":unsupported flags 0x%04x\n",flags);

  hand = CreateFileA(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, creation, FILE_ATTRIBUTE_NORMAL, 0);

  if (hand == INVALID_HANDLE_VALUE)
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    MSVCRT__set_errno(GetLastError());
    return -1;
  }

  fd = MSVCRT__alloc_fd(hand, ioflag);

  TRACE(":fd (%d) handle (%d)\n",fd, hand);

  if (fd > 0)
  {
    if (flags & _O_TEMPORARY)
      MSVCRT_tempfiles[fd] = MSVCRT__strdup(path);
    if (ioflag & _IOAPPEND)
      MSVCRT__lseek(fd, 0, FILE_END);
  }

  return fd;
}

/*********************************************************************
 *		_creat (MSVCRT.@)
 */
int __cdecl MSVCRT__creat(const char *path, int flags)
{
  int usedFlags = (flags & _O_TEXT)| _O_CREAT| _O_WRONLY| _O_TRUNC;
  return MSVCRT__open(path, usedFlags);
}

/*********************************************************************
 *		_open_osfhandle (MSVCRT.@)
 */
int __cdecl MSVCRT__open_osfhandle(HANDLE hand, int flags)
{
  int fd = MSVCRT__alloc_fd(hand,flags);
  TRACE(":handle (%d) fd (%d)\n",hand,fd);
  return fd;
}

/*********************************************************************
 *		_rmtmp (MSVCRT.@)
 */
int __cdecl MSVCRT__rmtmp(void)
{
  int num_removed = 0, i;

  for (i = 3; i < MSVCRT_fdend; i++)
    if (MSVCRT_tempfiles[i])
    {
      MSVCRT__close(i);
      num_removed++;
    }

  if (num_removed)
    TRACE(":removed (%d) temp files\n",num_removed);
  return num_removed;
}

/*********************************************************************
 *		_read (MSVCRT.@)
 */
int __cdecl MSVCRT__read(int fd, LPVOID buf, unsigned int count)
{
  DWORD num_read;
  HANDLE hand = MSVCRT__fdtoh(fd);

  /* Dont trace small reads, it gets *very* annoying */
  if (count > 4)
    TRACE(":fd (%d) handle (%d) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Set _cnt to 0 so optimised binaries will call our implementation
   * of putc/getc. See _filbuf/_flsbuf comments.
   */
  if (MSVCRT_files[fd])
    MSVCRT_files[fd]->_cnt = 0;

  if (ReadFile(hand, buf, count, &num_read, NULL))
  {
    if (num_read != count && MSVCRT_files[fd])
    {
      TRACE(":EOF\n");
      MSVCRT_files[fd]->_flag |= _IOEOF;
    }
    return num_read;
  }
  TRACE(":failed-last error (%ld)\n",GetLastError());
  if (MSVCRT_files[fd])
     MSVCRT_files[fd]->_flag |= _IOERR;
  return -1;
}

/*********************************************************************
 *		_getw (MSVCRT.@)
 */
int __cdecl MSVCRT__getw(MSVCRT_FILE* file)
{
  int i;
  if (MSVCRT__read(file->_file, &i, sizeof(int)) != 1)
    return MSVCRT_EOF;
  return i;
}

/*********************************************************************
 *		_setmode (MSVCRT.@)
 */
int __cdecl MSVCRT__setmode(int fd,int mode)
{
  if (mode & _O_TEXT)
    FIXME("fd (%d) mode (%d) TEXT not implemented\n",fd,mode);
  return 0;
}

/*********************************************************************
 *		_stat (MSVCRT.@)
 */
int __cdecl MSVCRT__stat(const char* path, struct _stat * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = MSVCRT_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &hfi))
  {
      TRACE("failed-last error (%ld)\n",GetLastError());
      MSVCRT__set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct _stat));

  /* FIXME: rdev isnt drive num,despite what the docs say-what is it? */
  if (isalpha(*path))
    buf->st_dev = buf->st_rdev = toupper(*path - 'A'); /* drive num */
  else
    buf->st_dev = buf->st_rdev = MSVCRT__getdrive() - 1;

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
      unsigned int ext = tolower(path[plen-1]) | (tolower(path[plen-2]) << 8)
	| (tolower(path[plen-3]) << 16);
      if (ext == EXE || ext == BAT || ext == CMD || ext == COM)
	mode |= MSVCRT_S_IEXEC;
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= MSVCRT_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
  buf->st_size  = hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970(&hfi.ftLastAccessTime, &dw);
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970(&hfi.ftLastWriteTime, &dw);
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("\n%d %d %d %d %d %d\n", buf->st_mode,buf->st_nlink,buf->st_size,
	buf->st_atime,buf->st_mtime, buf->st_ctime);
  return 0;
}

/*********************************************************************
 *		_tell (MSVCRT.@)
 */
LONG __cdecl MSVCRT__tell(int fd)
{
  return MSVCRT__lseek(fd, 0, SEEK_CUR);
}

/*********************************************************************
 *		_tempnam (MSVCRT.@)
 */
char *__cdecl MSVCRT__tempnam(const char *dir, const char *prefix)
{
  char tmpbuf[MAX_PATH];

  TRACE("dir (%s) prefix (%s)\n",dir,prefix);
  if (GetTempFileNameA(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",tmpbuf);
    return MSVCRT__strdup(tmpbuf);
  }
  TRACE("failed-last error (%ld)\n",GetLastError());
  return NULL;
}

/*********************************************************************
 *		_umask (MSVCRT.@)
 */
int __cdecl MSVCRT__umask(int umask)
{
  int old_umask = MSVCRT_umask;
  TRACE("umask (%d)\n",umask);
  MSVCRT_umask = umask;
  return old_umask;
}

/*********************************************************************
 *		_utime (MSVCRT.@)
 */
int __cdecl MSVCRT__utime(const char *path, struct _utimbuf *t)
{
  int fd = MSVCRT__open(path, _O_WRONLY | _O_BINARY);

  if (fd > 0)
  {
    int retVal = MSVCRT__futime(fd, t);
    MSVCRT__close(fd);
    return retVal;
  }
  return -1;
}

/*********************************************************************
 *		_write (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT__write(int fd, LPCVOID buf, unsigned int count)
{
  DWORD num_written;
  HANDLE hand = MSVCRT__fdtoh(fd);

  /* Dont trace small writes, it gets *very* annoying */
//  if (count > 32)
//    TRACE(":fd (%d) handle (%d) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* If appending, go to EOF */
  if (MSVCRT_flags[fd] & _IOAPPEND)
    MSVCRT__lseek(fd, 0, FILE_END);

  /* Set _cnt to 0 so optimised binaries will call our implementation
   * of putc/getc.
   */
  if (MSVCRT_files[fd])
    MSVCRT_files[fd]->_cnt = 0;

  if (WriteFile(hand, buf, count, &num_written, NULL)
      &&  (num_written == count))
    return num_written;

  TRACE(":failed-last error (%ld)\n",GetLastError());
  if (MSVCRT_files[fd])
     MSVCRT_files[fd]->_flag |= _IOERR;

  return -1;
}

/*********************************************************************
 *		_putw (MSVCRT.@)
 */
int __cdecl MSVCRT__putw(int val, MSVCRT_FILE* file)
{
  return MSVCRT__write(file->_file, &val, sizeof(val)) == 1? val : MSVCRT_EOF;
}

/*********************************************************************
 *		clearerr (MSVCRT.@)
 */
VOID __cdecl MSVCRT_clearerr(MSVCRT_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);
  file->_flag &= ~(_IOERR | _IOEOF);
}

/*********************************************************************
 *		fclose (MSVCRT.@)
 */
int __cdecl MSVCRT_fclose(MSVCRT_FILE* file)
{
  return MSVCRT__close(file->_file);
}

/*********************************************************************
 *		feof (MSVCRT.@)
 */
int __cdecl MSVCRT_feof(MSVCRT_FILE* file)
{
  return file->_flag & _IOEOF;
}

/*********************************************************************
 *		ferror (MSVCRT.@)
 */
int __cdecl MSVCRT_ferror(MSVCRT_FILE* file)
{
  return file->_flag & _IOERR;
}

/*********************************************************************
 *		fflush (MSVCRT.@)
 */
int __cdecl MSVCRT_fflush(MSVCRT_FILE* file)
{
  return MSVCRT__commit(file->_file);
}

/*********************************************************************
 *		fgetc (MSVCRT.@)
 */
int __cdecl MSVCRT_fgetc(MSVCRT_FILE* file)
{
  char c;
  if (MSVCRT__read(file->_file,&c,1) != 1)
    return MSVCRT_EOF;
  return c;
}

/*********************************************************************
 *		_fgetchar (MSVCRT.@)
 */
int __cdecl MSVCRT__fgetchar(void)
{
  return MSVCRT_fgetc(MSVCRT_stdin);
}

/*********************************************************************
 *		_filbuf (MSVCRT.@)
 */
int __cdecl MSVCRT__filbuf(MSVCRT_FILE* file)
{
  return MSVCRT_fgetc(file);
}

/*********************************************************************
 *		fgetpos (MSVCRT.@)
 */
int __cdecl MSVCRT_fgetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
  *pos = MSVCRT__tell(file->_file);
  return (*pos == -1? -1 : 0);
}

/*********************************************************************
 *		fgets (MSVCRT.@)
 */
CHAR* __cdecl MSVCRT_fgets(char *s, int size, MSVCRT_FILE* file)
{
  int    cc;
  char * buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
	file,file->_file,s,size);

  /* BAD, for the whole WINE process blocks... just done this way to test
   * windows95's ftp.exe.
   * JG - Is this true now we use ReadFile() on stdin too?
   */
  for(cc = MSVCRT_fgetc(file); cc != MSVCRT_EOF && cc != '\n';
      cc = MSVCRT_fgetc(file))
    if (cc != '\r')
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
  TRACE(":got '%s'\n", buf_start);
  return buf_start;
}

/*********************************************************************
 *		fgetwc (MSVCRT.@)
 */
WCHAR __cdecl MSVCRT_fgetwc(MSVCRT_FILE* file)
{
  WCHAR wc;
  if (MSVCRT__read(file->_file, &wc, sizeof(wc)) != sizeof(wc))
    return MSVCRT_WEOF;
  return wc;
}

/*********************************************************************
 *		getwc (MSVCRT.@)
 */
WCHAR __cdecl MSVCRT_getwc(MSVCRT_FILE* file)
{
  return MSVCRT_fgetwc(file);
}

/*********************************************************************
 *		_fgetwchar (MSVCRT.@)
 */
WCHAR __cdecl MSVCRT__fgetwchar(void)
{
  return MSVCRT_fgetwc(MSVCRT_stdin);
}

/*********************************************************************
 *		getwchar (MSVCRT.@)
 */
WCHAR __cdecl MSVCRT_getwchar(void)
{
  return MSVCRT__fgetwchar();
}

/*********************************************************************
 *		fputwc (MSVCRT.@)
 */
WCHAR __cdecl MSVCRT_fputwc(WCHAR wc, MSVCRT_FILE* file)
{
  if (MSVCRT__write(file->_file, &wc, sizeof(wc)) != sizeof(wc))
    return MSVCRT_WEOF;
  return wc;
}

/*********************************************************************
 *		_fputwchar (MSVCRT.@)
 */
WCHAR __cdecl MSVCRT__fputwchar(WCHAR wc)
{
  return MSVCRT_fputwc(wc, MSVCRT_stdout);
}

/*********************************************************************
 *		fopen (MSVCRT.@)
 */
MSVCRT_FILE* __cdecl MSVCRT_fopen(const char *path, const char *mode)
{
  MSVCRT_FILE* file;
  int flags = 0, plus = 0, fd;
  const char* search = mode;

  TRACE(":path (%s) mode (%s)\n",path,mode);

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

  fd = MSVCRT__open(path, flags);

  if (fd < 0)
    return NULL;

  file = MSVCRT__alloc_fp(fd);
  TRACE(":get file (%p)\n",file);
  if (!file)
    MSVCRT__close(fd);

  return file;
}

/*********************************************************************
 *		_fsopen (MSVCRT.@)
 */
MSVCRT_FILE*  __cdecl MSVCRT__fsopen(const char *path, const char *mode, int share)
{
  FIXME(":(%s,%s,%d),ignoring share mode!\n",path,mode,share);
  return MSVCRT_fopen(path,mode);
}

/*********************************************************************
 *		fputc (MSVCRT.@)
 */
int __cdecl MSVCRT_fputc(int c, MSVCRT_FILE* file)
{
  return MSVCRT__write(file->_file, &c, 1) == 1? c : MSVCRT_EOF;
}

/*********************************************************************
 *		_flsbuf (MSVCRT.@)
 */
int __cdecl MSVCRT__flsbuf(int c, MSVCRT_FILE* file)
{
  return MSVCRT_fputc(c,file);
}

/*********************************************************************
 *		_fputchar (MSVCRT.@)
 */
int __cdecl MSVCRT__fputchar(int c)
{
  return MSVCRT_fputc(c, MSVCRT_stdout);
}

/*********************************************************************
 *		fread (MSVCRT.@)
 */
DWORD __cdecl MSVCRT_fread(LPVOID ptr, int size, int nmemb, MSVCRT_FILE* file)
{
  DWORD read = MSVCRT__read(file->_file,ptr, size * nmemb);
  if (read <= 0)
    return 0;
  return read / size;
}

/*********************************************************************
 *		freopen (MSVCRT.@)
 *
 */
MSVCRT_FILE* __cdecl MSVCRT_freopen(const char *path, const char *mode,MSVCRT_FILE* file)
{
  MSVCRT_FILE* newfile;
  int fd;

  TRACE(":path (%p) mode (%s) file (%p) fd (%d)\n",path,mode,file,file->_file);
  if (!file || ((fd = file->_file) < 0) || fd > MSVCRT_fdend)
    return NULL;

  if (fd > 2)
  {
    FIXME(":reopen on user file not implemented!\n");
    MSVCRT__set_errno(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
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
    MSVCRT__free_fd(newfile->_file);
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
int __cdecl MSVCRT_fsetpos(MSVCRT_FILE* file, MSVCRT_fpos_t *pos)
{
  return MSVCRT__lseek(file->_file,*pos,SEEK_SET);
}

/*********************************************************************
 *		fscanf (MSVCRT.@)
 */
int __cdecl MSVCRT_fscanf(MSVCRT_FILE* file, const char *format, ...)
{
    /* NOTE: If you extend this function, extend MSVCRT__cscanf in console.c too */
    int rd = 0;
    int nch;
    va_list ap;
    if (!*format) return 0;
    WARN("%p (\"%s\"): semi-stub\n", file, format);
    nch = MSVCRT_fgetc(file);
    va_start(ap, format);
    while (*format) {
        if (*format == ' ') {
            /* skip whitespace */
            while ((nch!=MSVCRT_EOF) && isspace(nch))
                nch = MSVCRT_fgetc(file);
        }
        else if (*format == '%') {
            int st = 0;
            format++;
            switch(*format) {
            case 'd': { /* read an integer */
                    int*val = va_arg(ap, int*);
                    int cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = MSVCRT_fgetc(file);
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = MSVCRT_fgetc(file);
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    nch = MSVCRT_fgetc(file);
                    /* read until no more digits */
                    while ((nch!=MSVCRT_EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = MSVCRT_fgetc(file);
                    }
                    st = 1;
                    *val = cur;
                }
                break;
            case 'f': { /* read a float */
                    float*val = va_arg(ap, float*);
                    float cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = MSVCRT_fgetc(file);
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = MSVCRT_fgetc(file);
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    /* read until no more digits */
                    while ((nch!=MSVCRT_EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = MSVCRT_fgetc(file);
                    }
                    if (nch == '.') {
                        /* handle decimals */
                        float dec = 1;
                        nch = MSVCRT_fgetc(file);
                        while ((nch!=MSVCRT_EOF) && isdigit(nch)) {
                            dec /= 10;
                            cur += dec * (nch - '0');
                            nch = MSVCRT_fgetc(file);
                        }
                    }
                    st = 1;
                    *val = cur;
                }
                break;
            case 's': { /* read a word */
                    char*str = va_arg(ap, char*);
                    char*sptr = str;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = MSVCRT_fgetc(file);
                    /* read until whitespace */
                    while ((nch!=MSVCRT_EOF) && !isspace(nch)) {
                        *sptr++ = nch; st++;
                        nch = MSVCRT_fgetc(file);
                    }
                    /* terminate */
                    *sptr = 0;
                    TRACE("read word: %s\n", str);
                }
                break;
            default: FIXME("unhandled: %%%c\n", *format);
            }
            if (st) rd++;
            else break;
        }
        else {
            /* check for character match */
            if (nch == *format)
               nch = MSVCRT_fgetc(file);
            else break;
        }
        format++;
    }
    va_end(ap);
    if (nch!=MSVCRT_EOF) {
        WARN("need ungetch\n");
    }
    TRACE("returning %d\n", rd);
    return rd;
}

/*********************************************************************
 *		fseek (MSVCRT.@)
 */
LONG __cdecl MSVCRT_fseek(MSVCRT_FILE* file, LONG offset, int whence)
{
  return MSVCRT__lseek(file->_file,offset,whence);
}

/*********************************************************************
 *		ftell (MSVCRT.@)
 */
LONG __cdecl MSVCRT_ftell(MSVCRT_FILE* file)
{
  return MSVCRT__tell(file->_file);
}

/*********************************************************************
 *		fwrite (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT_fwrite(LPCVOID ptr, int size, int nmemb, MSVCRT_FILE* file)
{
  unsigned int written = MSVCRT__write(file->_file, ptr, size * nmemb);
  if (written <= 0)
    return 0;
  return written / size;
}

/*********************************************************************
 *		fputs (MSVCRT.@)
 */
int __cdecl MSVCRT_fputs(const char *s, MSVCRT_FILE* file)
{
  return MSVCRT_fwrite(s,strlen(s),1,file) == 1 ? 0 : MSVCRT_EOF;
}

/*********************************************************************
 *		getchar (MSVCRT.@)
 */
int __cdecl MSVCRT_getchar(void)
{
  return MSVCRT_fgetc(MSVCRT_stdin);
}

/*********************************************************************
 *		getc (MSVCRT.@)
 */
int __cdecl MSVCRT_getc(MSVCRT_FILE* file)
{
  return MSVCRT_fgetc(file);
}

/*********************************************************************
 *		gets (MSVCRT.@)
 */
char *__cdecl MSVCRT_gets(char *buf)
{
  int    cc;
  char * buf_start = buf;

  /* BAD, for the whole WINE process blocks... just done this way to test
   * windows95's ftp.exe.
   * JG 19/9/00: Is this still true, now we are using ReadFile?
   */
  for(cc = MSVCRT_fgetc(MSVCRT_stdin); cc != MSVCRT_EOF && cc != '\n';
      cc = MSVCRT_fgetc(MSVCRT_stdin))
  if(cc != '\r') *buf++ = (char)cc;

  *buf = '\0';

  TRACE("got '%s'\n", buf_start);
  return buf_start;
}

/*********************************************************************
 *		putc (MSVCRT.@)
 */
int __cdecl MSVCRT_putc(int c, MSVCRT_FILE* file)
{
  return MSVCRT_fputc(c, file);
}

/*********************************************************************
 *		putchar (MSVCRT.@)
 */
void __cdecl MSVCRT_putchar(int c)
{
  MSVCRT_fputc(c, MSVCRT_stdout);
}

/*********************************************************************
 *		puts (MSVCRT.@)
 */
int __cdecl MSVCRT_puts(const char *s)
{
  int retval = MSVCRT_EOF;
  if (MSVCRT_fwrite(s,strlen(s),1,MSVCRT_stdout) == 1)
    return MSVCRT_fwrite("\n",1,1,MSVCRT_stdout) == 1 ? 0 : MSVCRT_EOF;
  return retval;
}

/*********************************************************************
 *		remove (MSVCRT.@)
 */
int __cdecl MSVCRT_remove(const char *path)
{
  TRACE(":path (%s)\n",path);
  if (DeleteFileA(path))
    return 0;
  TRACE(":failed-last error (%ld)\n",GetLastError());
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		scanf (MSVCRT.@)
 */
int __cdecl MSVCRT_scanf(const char *format, ...)
{
  va_list valist;
  int res;

  va_start(valist, format);
  res = MSVCRT_fscanf(MSVCRT_stdin, format, valist);
  va_end(valist);
  return res;
}

/*********************************************************************
 *		rename (MSVCRT.@)
 */
int __cdecl MSVCRT_rename(const char *oldpath,const char *newpath)
{
  TRACE(":from %s to %s\n",oldpath,newpath);
  if (MoveFileExA(oldpath, newpath, MOVEFILE_REPLACE_EXISTING))
    return 0;
  TRACE(":failed-last error (%ld)\n",GetLastError());
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		setbuf (MSVCRT.@)
 */
int __cdecl MSVCRT_setbuf(MSVCRT_FILE* file, char *buf)
{
  TRACE(":file (%p) fd (%d) buf (%p)\n", file, file->_file,buf);
  if (buf)
    WARN(":user buffer will not be used!\n");
  /* FIXME: no buffering for now */
  return 0;
}

/*********************************************************************
 *		tmpnam (MSVCRT.@)
 */
char *__cdecl MSVCRT_tmpnam(char *s)
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
MSVCRT_FILE* __cdecl MSVCRT_tmpfile(void)
{
  char *filename = MSVCRT_tmpnam(NULL);
  int fd;
  fd = MSVCRT__open(filename, _O_CREAT | _O_BINARY | _O_RDWR | _O_TEMPORARY);
  if (fd != -1)
    return MSVCRT__alloc_fp(fd);
  return NULL;
}
extern int vsnprintf(void *, unsigned int, const void*, va_list);

/*********************************************************************
 *		vfprintf (MSVCRT.@)
 */
int __cdecl MSVCRT_vfprintf(MSVCRT_FILE* file, const char *format, va_list valist)
{
  char buf[2048], *mem = buf;
  int written, resize = sizeof(buf);
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
  if (mem != buf)
    MSVCRT_free (mem);
  return MSVCRT_fwrite(mem, 1, written, file);
}

/*********************************************************************
 *		fprintf (MSVCRT.@)
 */
int __cdecl MSVCRT_fprintf(MSVCRT_FILE* file, const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = MSVCRT_vfprintf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		printf (MSVCRT.@)
 */
int __cdecl MSVCRT_printf(const char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = MSVCRT_vfprintf(MSVCRT_stdout, format, valist);
    va_end(valist);
    return res;
}

