/*
 * CRTDLL file functions
 * 
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * Implementation Notes:
 * Mapping is performed between FILE*, fd and HANDLE's. This allows us to 
 * implement all calls using the Win32 API, support remapping fd's to 
 * FILES and do some other tricks as well (like closeall, _get_osfhandle).
 * For mix and matching with the host libc, processes can use the Win32 HANDLE
 * to get a real unix fd from the wineserver. Or we could do this once
 * on create, and provide a function to return it quickly (store it
 * in the mapping table). Note that If you actuall _do_ this, you should
 * call rewind() before using any other crt functions on the file. To avoid
 * the confusion I got when reading the API docs, fd is always refered
 * to as a file descriptor here. In the API docs its called a file handle
 * which is confusing with Win32 HANDLES.
 * M$ CRT includes inline versions of some of these functions (like feof()).
 * These inlines check/modify bitfields in the FILE structure, so we set
 * _flags/_file/_cnt in the FILE* to be binary compatable with the win dll.
 * lcc defines _IOAPPEND as one of the flags for a FILE*, but testing shows
 * that M$ CRT never sets it. So we keep the flag in our mapping table but
 * mask it out when we populate a FILE* with it. Then when we write we seek
 * to EOF if _IOAPPEND is set for the underlying fd.
 *
 * FIXME:
 * Not MT safe. Need locking around file access and allocation for this.
 * NT has no effective limit on files - neither should we. This will be fixed
 * with dynamic allocation of the file mapping array.
 * Buffering is handled differently. Have to investigate a) how much control
 * we have over buffering in win32, and b) if we care ;-)
 */

#include "crtdll.h"
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "ntddk.h"

DEFAULT_DEBUG_CHANNEL(crtdll);

/* FIXME: Make this dynamic */
#define CRTDLL_MAX_FILES 257

HANDLE __CRTDLL_handles[CRTDLL_MAX_FILES];
CRTDLL_FILE* __CRTDLL_files[CRTDLL_MAX_FILES];
INT  __CRTDLL_flags[CRTDLL_MAX_FILES];
CRTDLL_FILE __CRTDLL_iob[3];

static int __CRTDLL_fdstart = 3; /* first unallocated fd */
static int __CRTDLL_fdend = 3; /* highest allocated fd */

/* INTERNAL: process umask */
static INT __CRTDLL_umask = 0;

/* INTERNAL: Static buffer for temp file name */
static char CRTDLL_tmpname[MAX_PATH];

/* file extentions recognised as executables */
static const unsigned int EXE = 'e' << 16 | 'x' << 8 | 'e';
static const unsigned int BAT = 'b' << 16 | 'a' << 8 | 't';
static const unsigned int CMD = 'c' << 16 | 'm' << 8 | 'd';
static const unsigned int COM = 'c' << 16 | 'o' << 8 | 'm';

/* for stat mode, permissions apply to all,owner and group */
#define CRTDLL_S_IREAD  (_S_IREAD  | (_S_IREAD  >> 3) | (_S_IREAD  >> 6))
#define CRTDLL_S_IWRITE (_S_IWRITE | (_S_IWRITE >> 3) | (_S_IWRITE >> 6))
#define CRTDLL_S_IEXEC  (_S_IEXEC  | (_S_IEXEC  >> 3) | (_S_IEXEC  >> 6))


/* INTERNAL: Get the HANDLE for a fd */
static HANDLE __CRTDLL__fdtoh(INT fd);
static HANDLE __CRTDLL__fdtoh(INT fd)
{
  if (fd < 0 || fd >= __CRTDLL_fdend ||
      __CRTDLL_handles[fd] == INVALID_HANDLE_VALUE)
  {
    WARN(":fd (%d) - no handle!\n",fd);
    CRTDLL_doserrno = 0;
    CRTDLL_errno = EBADF;
   return INVALID_HANDLE_VALUE;
  }
  return __CRTDLL_handles[fd];
}


/* INTERNAL: free a file entry fd */
static void __CRTDLL__free_fd(INT fd);
static void __CRTDLL__free_fd(INT fd)
{
  __CRTDLL_handles[fd] = INVALID_HANDLE_VALUE;
  __CRTDLL_files[fd] = 0;
  __CRTDLL_flags[fd] = 0;
  TRACE(":fd (%d) freed\n",fd);
  if (fd < 3)
    return; /* dont use 0,1,2 for user files */
  if (fd == __CRTDLL_fdend - 1)
    __CRTDLL_fdend--;
  if (fd < __CRTDLL_fdstart)
    __CRTDLL_fdstart = fd;
}


/* INTERNAL: Allocate an fd slot from a Win32 HANDLE */
static INT __CRTDLL__alloc_fd(HANDLE hand, INT flag);
static INT __CRTDLL__alloc_fd(HANDLE hand, INT flag)
{
  INT fd = __CRTDLL_fdstart;

  TRACE(":handle (%d) allocating fd (%d)\n",hand,fd);
  if (fd >= CRTDLL_MAX_FILES)
  {
    WARN(":files exhausted!\n");
    return -1;
  }
  __CRTDLL_handles[fd] = hand;
  __CRTDLL_flags[fd] = flag;

  /* locate next free slot */
  if (fd == __CRTDLL_fdend)
    __CRTDLL_fdstart = ++__CRTDLL_fdend;
  else
    while(__CRTDLL_fdstart < __CRTDLL_fdend &&
	  __CRTDLL_handles[__CRTDLL_fdstart] != INVALID_HANDLE_VALUE)
      __CRTDLL_fdstart++;

  return fd;
}


/* INTERNAL: Allocate a FILE* for an fd slot
 * This is done lazily to avoid memory wastage for low level open/write
 * usage when a FILE* is not requested (but may be later).
 */
static CRTDLL_FILE* __CRTDLL__alloc_fp(INT fd);
static CRTDLL_FILE* __CRTDLL__alloc_fp(INT fd)
{
  TRACE(":fd (%d) allocating FILE*\n",fd);
  if (fd < 0 || fd >= __CRTDLL_fdend || 
      __CRTDLL_handles[fd] == INVALID_HANDLE_VALUE)
  {
    WARN(":invalid fd %d\n",fd);
    CRTDLL_doserrno = 0;
    CRTDLL_errno = EBADF;
    return NULL;
  }
  if (!__CRTDLL_files[fd])
  {
    if ((__CRTDLL_files[fd] = CRTDLL_calloc(sizeof(CRTDLL_FILE),1)))
    {
      __CRTDLL_files[fd]->_file = fd;
      __CRTDLL_files[fd]->_flag = __CRTDLL_flags[fd];
      __CRTDLL_files[fd]->_flag &= ~_IOAPPEND; /* mask out, see above */
    }
  }
  TRACE(":got FILE* (%p)\n",__CRTDLL_files[fd]);
  return __CRTDLL_files[fd];
}


/* INTERNAL: Set up stdin, stderr and stdout */
VOID __CRTDLL__init_io(VOID)
{
  int i;
  memset(__CRTDLL_iob,0,3*sizeof(CRTDLL_FILE));
  __CRTDLL_handles[0] = GetStdHandle(STD_INPUT_HANDLE);
  __CRTDLL_flags[0] = __CRTDLL_iob[0]._flag = _IOREAD;
  __CRTDLL_handles[1] = GetStdHandle(STD_OUTPUT_HANDLE);
  __CRTDLL_flags[1] = __CRTDLL_iob[1]._flag = _IOWRT;
  __CRTDLL_handles[2] = GetStdHandle(STD_ERROR_HANDLE);
  __CRTDLL_flags[2] = __CRTDLL_iob[2]._flag = _IOWRT;

  TRACE(":handles (%d)(%d)(%d)\n",__CRTDLL_handles[0],
	__CRTDLL_handles[1],__CRTDLL_handles[2]);

  for (i = 0; i < 3; i++)
  {
    /* FILE structs for stdin/out/err are static and never deleted */
    __CRTDLL_files[i] = &__CRTDLL_iob[i];
    __CRTDLL_iob[i]._file = i;
  }
}


/*********************************************************************
 *                  _access          (CRTDLL.37)
 */
INT __cdecl CRTDLL__access(LPCSTR filename, INT mode)
{
  DWORD attr = GetFileAttributesA(filename);

  if (attr == 0xffffffff)
  {
    if (!filename)
    {
	/* FIXME: Should GetFileAttributesA() return this? */
      __CRTDLL__set_errno(ERROR_INVALID_DATA);
      return -1;
    }
    __CRTDLL__set_errno(GetLastError());
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & W_OK))
  {
    __CRTDLL__set_errno(ERROR_ACCESS_DENIED);
    return -1;
  }
  TRACE(":file %s, mode (%d) ok\n",filename,mode);
  return 0;
}


/*********************************************************************
 *                  _chmod           (CRTDLL.054)
 *
 * Change a files permissions.
 */
INT __cdecl CRTDLL__chmod(LPCSTR path, INT flags)
{
  DWORD oldFlags = GetFileAttributesA(path);
 
  if (oldFlags != 0x0FFFFFFFF)
  {
    DWORD newFlags = (flags & _S_IWRITE)? oldFlags & ~FILE_ATTRIBUTE_READONLY:
      oldFlags | FILE_ATTRIBUTE_READONLY;

    if (newFlags == oldFlags || SetFileAttributesA( path, newFlags ))
      return 0;
  }
  __CRTDLL__set_errno(GetLastError());
  return -1;
}


/*********************************************************************
 *                  _close           (CRTDLL.57)
 *
 * Close an open file descriptor.
 */
INT __cdecl CRTDLL__close(INT fd)
{
  HANDLE hand = __CRTDLL__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Dont free std FILE*'s, they are not dynamic */
  if (fd > 2 && __CRTDLL_files[fd])
    CRTDLL_free(__CRTDLL_files[fd]);

  __CRTDLL__free_fd(fd);

  if (!CloseHandle(hand))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    __CRTDLL__set_errno(GetLastError());
    return -1;
  }
  TRACE(":ok\n");
  return 0;
}


/*********************************************************************
 *                  _commit           (CRTDLL.58)
 *
 * Ensure all file operations have been flushed to the drive.
 */
INT __cdecl CRTDLL__commit(INT fd)
{
  HANDLE hand = __CRTDLL__fdtoh(fd);

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
    __CRTDLL__set_errno(GetLastError());
    return -1;
  }
  TRACE(":ok\n");
  return 0;
}


/*********************************************************************
 *                  _creat         (CRTDLL.066)
 *
 * Open a file, creating it if it is not present.
 */
INT __cdecl CRTDLL__creat(LPCSTR path, INT flags)
{
  INT usedFlags = (flags & _O_TEXT)| _O_CREAT| _O_WRONLY| _O_TRUNC;
  return CRTDLL__open(path, usedFlags);
}


/*********************************************************************
 *                  _eof           (CRTDLL.076)
 *
 * Determine if the file pointer is at the end of a file.
 */
/* FIXME: Care for large files */
INT __cdecl CRTDLL__eof( INT fd )
{
  DWORD curpos,endpos;
  HANDLE hand = __CRTDLL__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);

  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* If we have a FILE* for this file, the EOF flag
   * will be set by the read()/write() functions.
   */
  if (__CRTDLL_files[fd])
    return __CRTDLL_files[fd]->_flag & _IOEOF;

  /* Otherwise we do it the hard way */
  curpos = SetFilePointer( hand, 0, NULL, SEEK_CUR );
  endpos = SetFilePointer( hand, 0, NULL, FILE_END );

  if (curpos == endpos)
    return TRUE;

  SetFilePointer( hand, curpos, 0, FILE_BEGIN);
  return FALSE;
}


/*********************************************************************
 *                  _fcloseall     (CRTDLL.089)
 *
 * Close all open files except stdin/stdout/stderr.
 */
INT __cdecl CRTDLL__fcloseall(VOID)
{
  int num_closed = 0, i = 3;

  while(i < __CRTDLL_fdend)
    if (__CRTDLL_handles[i] != INVALID_HANDLE_VALUE)
    {
      CRTDLL__close(i);
      num_closed++;
    }

  TRACE(":closed (%d) handles\n",num_closed);
  return num_closed;
}


/*********************************************************************
 *                  _fdopen     (CRTDLL.091)
 *
 * Get a FILE* from a low level file descriptor.
 */
CRTDLL_FILE* __cdecl CRTDLL__fdopen(INT fd, LPCSTR mode)
{
  CRTDLL_FILE* file = __CRTDLL__alloc_fp(fd);

  TRACE(":fd (%d) mode (%s) FILE* (%p)\n",fd,mode,file);
  if (file)
    CRTDLL_rewind(file);

  return file;
}


/*********************************************************************
 *                  _fgetchar       (CRTDLL.092)
 */
INT __cdecl CRTDLL__fgetchar( VOID )
{
  return CRTDLL_fgetc(CRTDLL_stdin);
}


/*********************************************************************
 *                  _filbuf     (CRTDLL.094)
 *
 * NOTES
 * The macro version of getc calls this function whenever FILE->_cnt
 * becomes negative. We ensure that _cnt is always 0 after any read
 * so this function is always called. Our implementation simply calls
 * fgetc as all the underlying buffering is handled by Wines 
 * implementation of the Win32 file I/O calls.
 */
INT __cdecl CRTDLL__filbuf(CRTDLL_FILE* file)
{
  return CRTDLL_fgetc(file);
}

/*********************************************************************
 *                   _filelength    (CRTDLL.097)
 *
 * Get the length of an open file.
 */
LONG __cdecl CRTDLL__filelength(INT fd)
{
  LONG curPos = CRTDLL__lseek(fd, 0, SEEK_CUR);
  if (curPos != -1)
  {
    LONG endPos = CRTDLL__lseek(fd, 0, SEEK_END);
    if (endPos != -1)
    {
      if (endPos != curPos)
        CRTDLL__lseek(fd, curPos, SEEK_SET);
      return endPos;
    }
  }
  return -1;
}


/*********************************************************************
 *                  _fileno     (CRTDLL.097)
 *
 * Get the file descriptor from a FILE*.
 *
 * NOTES
 * This returns the CRTDLL fd, _not_ the underlying *nix fd.
 */
INT __cdecl CRTDLL__fileno(CRTDLL_FILE* file)
{
  TRACE(":FILE* (%p) fd (%d)\n",file,file->_file);
  return file->_file;
}


/*********************************************************************
 *                  _flsbuf     (CRTDLL.102)
 *
 * NOTES
 * The macro version of putc calls this function whenever FILE->_cnt
 * becomes negative. We ensure that _cnt is always 0 after any write
 * so this function is always called. Our implementation simply calls
 * fputc as all the underlying buffering is handled by Wines
 * implementation of the Win32 file I/O calls.
 */
INT __cdecl CRTDLL__flsbuf(INT c, CRTDLL_FILE* file)
{
  return CRTDLL_fputc(c,file);
}


/*********************************************************************
 *                  _flushall     (CRTDLL.103)
 *
 * Flush all open files.
 */
INT __cdecl CRTDLL__flushall(VOID)
{
  int num_flushed = 0, i = 3;

  while(i < __CRTDLL_fdend)
    if (__CRTDLL_handles[i] != INVALID_HANDLE_VALUE)
    {
      if (CRTDLL__commit(i) == -1)
	if (__CRTDLL_files[i])
	  __CRTDLL_files[i]->_flag |= _IOERR;
      num_flushed++;
    }

  TRACE(":flushed (%d) handles\n",num_flushed);
  return num_flushed;
}


/*********************************************************************
 *                  _fputchar     (CRTDLL.108)
 *
 * Put a character to a file.
 */
INT __cdecl CRTDLL__fputchar(INT c)
{
  return CRTDLL_fputc(c, CRTDLL_stdout);
}


/*********************************************************************
 *                  _fsopen     (CRTDLL.110)
 *
 * Open a FILE* with sharing.
 */
CRTDLL_FILE*  __cdecl CRTDLL__fsopen(LPCSTR path, LPCSTR mode, INT share)
{
  FIXME(":(%s,%s,%d),ignoring share mode!\n",path,mode,share);
  return CRTDLL_fopen(path,mode);
}


/*********************************************************************
 *                  _fstat        (CRTDLL.111)
 * 
 * Get information about an open file.
 */
int __cdecl CRTDLL__fstat(int fd, struct _stat* buf)
{
  DWORD dw;
  BY_HANDLE_FILE_INFORMATION hfi;
  HANDLE hand = __CRTDLL__fdtoh(fd);

  TRACE(":fd (%d) stat (%p)\n",fd,buf);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (!buf)
  {
    WARN(":failed-NULL buf\n");
    __CRTDLL__set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }

  memset(&hfi, 0, sizeof(hfi));
  memset(buf, 0, sizeof(struct _stat));
  if (!GetFileInformationByHandle(hand, &hfi))
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    __CRTDLL__set_errno(ERROR_INVALID_PARAMETER);
    return -1;
  }
  FIXME(":dwFileAttributes = %d, mode set to 0",hfi.dwFileAttributes);
  buf->st_nlink = hfi.nNumberOfLinks;
  buf->st_size  = hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970( &hfi.ftLastAccessTime, &dw );
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970( &hfi.ftLastWriteTime, &dw );
  buf->st_mtime = buf->st_ctime = dw;
  return 0;
}


/*********************************************************************
 *                  _futime        (CRTDLL.115)
 * 
 * Set the file access/modification times on an open file.
 */
INT __cdecl CRTDLL__futime(INT fd, struct _utimbuf *t)
{
  HANDLE hand = __CRTDLL__fdtoh(fd);
  FILETIME at, wt;

  if (!t)
  {
    time_t currTime;
    CRTDLL_time(&currTime);
    RtlSecondsSince1970ToTime( currTime, &at );
    memcpy( &wt, &at, sizeof(wt) );
  }
  else
  {
    RtlSecondsSince1970ToTime( t->actime, &at );
    if (t->actime == t->modtime)
      memcpy( &wt, &at, sizeof(wt) );
    else
      RtlSecondsSince1970ToTime( t->modtime, &wt );
  }

  if (!SetFileTime( hand, NULL, &at, &wt ))
  {
    __CRTDLL__set_errno(GetLastError());
    return -1 ;
  }
  return 0;
}


/*********************************************************************
 *                  _get_osfhandle     (CRTDLL.117)
 *
 * Return a Win32 HANDLE from a file descriptor.
 *
 * PARAMS
 * fd [in] A valid file descriptor
 *
 * RETURNS
 * Success: A Win32 HANDLE
 *
 * Failure: INVALID_HANDLE_VALUE.
 *
 */
HANDLE CRTDLL__get_osfhandle(INT fd)
{
  HANDLE hand = __CRTDLL__fdtoh(fd);
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
		    &newhand,0,TRUE,DUPLICATE_SAME_ACCESS );
  }
  return newhand;
}


/*********************************************************************
 *                  _getw       (CRTDLL.128)
 *
 * Read an integter from a FILE*.
 */
INT __cdecl CRTDLL__getw( CRTDLL_FILE* file )
{
  INT i;
  if (CRTDLL__read(file->_file, &i, sizeof(INT)) != 1)
    return EOF;
  return i;
}


/*********************************************************************
 *                  _isatty       (CRTDLL.137)
 *
 * Return non zero if fd is a character device (e.g console).
 */
INT __cdecl CRTDLL__isatty(INT fd)
{
  HANDLE hand = __CRTDLL__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return 0;

  return GetFileType(fd) == FILE_TYPE_CHAR? 1 : 0;
}


/*********************************************************************
 *                  _lseek     (CRTDLL.179)
 *
 * Move the file pointer within a file.
 */
LONG __cdecl CRTDLL__lseek( INT fd, LONG offset, INT whence)
{
  DWORD ret;
  HANDLE hand = __CRTDLL__fdtoh(fd);

  TRACE(":fd (%d) handle (%d)\n",fd,hand);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  if (whence < 0 || whence > 2)
  {
    CRTDLL_errno = EINVAL;
    return -1;
  }

  TRACE(":fd (%d) to 0x%08lx pos %s\n",
        fd,offset,(whence==SEEK_SET)?"SEEK_SET":
        (whence==SEEK_CUR)?"SEEK_CUR":
        (whence==SEEK_END)?"SEEK_END":"UNKNOWN");

  if ((ret = SetFilePointer( hand, offset, NULL, whence )) != 0xffffffff)
  {
    if ( __CRTDLL_files[fd])
      __CRTDLL_files[fd]->_flag &= ~_IOEOF;
    /* FIXME: What if we seek _to_ EOF - is EOF set? */
    return ret;
  }
  TRACE(":error-last error (%ld)\n",GetLastError());
  if ( __CRTDLL_files[fd])
    switch(GetLastError())
    {
    case ERROR_NEGATIVE_SEEK:
    case ERROR_SEEK_ON_DEVICE:
      __CRTDLL__set_errno(GetLastError());
      __CRTDLL_files[fd]->_flag |= _IOERR;
      break;
    default:
      break;
    }
  return -1;
}

/*********************************************************************
 *                  _mktemp           (CRTDLL.239)
 *
 * Create a temporary file name.
 */
LPSTR __cdecl CRTDLL__mktemp(LPSTR pattern)
{
  int numX = 0;
  LPSTR retVal = pattern;
  INT id;
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
    INT tempNum = id / 10;
    *pattern-- = id - (tempNum * 10) + '0';
    id = tempNum;
  }
  pattern++;
  do
  {
    if (GetFileAttributesA( retVal ) == 0xFFFFFFFF &&
	GetLastError() == ERROR_FILE_NOT_FOUND)
      return retVal;
    *pattern = letter++;
  } while(letter != '|');
  return NULL;
}

/*********************************************************************
 *                  _open           (CRTDLL.239)
 * Open a file.
 */
INT __cdecl CRTDLL__open(LPCSTR path,INT flags)
{
  DWORD access = 0, creation = 0;
  INT ioflag = 0, fd;
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

  if (flags & ~(_O_BINARY|_O_TEXT|_O_APPEND|_O_TRUNC|_O_EXCL|_O_CREAT|_O_RDWR))
    TRACE(":unsupported flags 0x%04x\n",flags);

  /* clear those pesky flags ;-) */
  flags &= (_O_BINARY|_O_TEXT|_O_APPEND|_O_TRUNC|_O_EXCL|_O_CREAT|_O_RDWR);

  hand = CreateFileA( path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, creation, FILE_ATTRIBUTE_NORMAL, -1);

  if (hand == INVALID_HANDLE_VALUE)
  {
    WARN(":failed-last error (%ld)\n",GetLastError());
    __CRTDLL__set_errno(GetLastError());
    return -1;
  }

  fd = __CRTDLL__alloc_fd(hand,ioflag);

  TRACE(":fd (%d) handle (%d)\n",fd, hand);

  if (flags & _IOAPPEND && fd > 0)
    CRTDLL__lseek(fd, 0, FILE_END );

  return fd;
}


/*********************************************************************
 *                  _open_osfhandle         (CRTDLL.240)
 *
 * Create a file descriptor for a file HANDLE.
 */
INT __cdecl CRTDLL__open_osfhandle(HANDLE hand, INT flags)
{
  INT fd = __CRTDLL__alloc_fd(hand,flags);
  TRACE(":handle (%d) fd (%d)\n",hand,fd);
  return fd;
}


/*********************************************************************
 *                  _putw         (CRTDLL.254)
 *
 * Write an int to a FILE*.
 */
INT __cdecl CRTDLL__putw(INT val, CRTDLL_FILE* file)
{
  return CRTDLL__write(file->_file, &val, sizeof(val)) == 1? val : EOF;
}


/*********************************************************************
 *                  _read     (CRTDLL.256)
 *
 * Read data from a file.
 */
INT __cdecl CRTDLL__read(INT fd, LPVOID buf, UINT count)
{
  DWORD num_read;
  HANDLE hand = __CRTDLL__fdtoh(fd);

  /* Dont trace small reads, it gets *very* annoying */
  if (count > 4)
    TRACE(":fd (%d) handle (%d) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* Set _cnt to 0 so optimised binaries will call our implementation
   * of putc/getc. See _filbuf/_flsbuf comments.
   */
  if (__CRTDLL_files[fd])
    __CRTDLL_files[fd]->_cnt = 0;

  if (ReadFile(hand, buf, count, &num_read, NULL))
  {
    if (num_read != count && __CRTDLL_files[fd])
    {
      TRACE(":EOF\n");
      __CRTDLL_files[fd]->_flag |= _IOEOF;
    }
    return num_read;
  }
  TRACE(":failed-last error (%ld)\n",GetLastError());
  if ( __CRTDLL_files[fd])
     __CRTDLL_files[fd]->_flag |= _IOERR;
  return -1;
}


/*********************************************************************
 *                  _setmode           (CRTDLL.265)
 *
 * FIXME: At present we ignore the request to translate CR/LF to LF.
 *
 * We always translate when we read with fgets, we never do with fread
 *
 */
INT __cdecl CRTDLL__setmode(INT fd,INT mode)
{
  if (mode & _O_TEXT)
    FIXME("fd (%d) mode (%d) TEXT not implemented\n",fd,mode);
  return 0;
}


/*********************************************************************
 *                  _stat          (CRTDLL.280)
 */
INT __cdecl CRTDLL__stat(const char* path, struct _stat * buf)
{
  DWORD dw;
  WIN32_FILE_ATTRIBUTE_DATA hfi;
  unsigned short mode = CRTDLL_S_IREAD;
  int plen;

  TRACE(":file (%s) buf(%p)\n",path,buf);

  if (!GetFileAttributesExA( path, GetFileExInfoStandard, &hfi ))
  {
      TRACE("failed-last error (%ld)\n",GetLastError());
      __CRTDLL__set_errno(ERROR_FILE_NOT_FOUND);
      return -1;
  }

  memset(buf,0,sizeof(struct _stat));

  /* FIXME: rdev isnt drive num,despite what the docs say-what is it? */
  if (isalpha(*path))
    buf->st_dev = buf->st_rdev = toupper(*path - 'A'); /* drive num */
  else
    buf->st_dev = buf->st_rdev = CRTDLL__getdrive() - 1;

  plen = strlen(path);

  /* Dir, or regular file? */
  if ((hfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
      (path[plen-1] == '\\'))
    mode |= (_S_IFDIR | CRTDLL_S_IEXEC);
  else
  {
    mode |= _S_IFREG;
    /* executable? */
    if (plen > 6 && path[plen-4] == '.')  /* shortest exe: "\x.exe" */
    {
      unsigned int ext = tolower(path[plen-1]) | (tolower(path[plen-2]) << 8)
	| (tolower(path[plen-3]) << 16);
      if (ext == EXE || ext == BAT || ext == CMD || ext == COM)
	mode |= CRTDLL_S_IEXEC;
    }
  }

  if (!(hfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    mode |= CRTDLL_S_IWRITE;

  buf->st_mode  = mode;
  buf->st_nlink = 1;
  buf->st_size  = hfi.nFileSizeLow;
  RtlTimeToSecondsSince1970( &hfi.ftLastAccessTime, &dw );
  buf->st_atime = dw;
  RtlTimeToSecondsSince1970( &hfi.ftLastWriteTime, &dw );
  buf->st_mtime = buf->st_ctime = dw;
  TRACE("\n%d %d %d %d %d %d\n", buf->st_mode,buf->st_nlink,buf->st_size,
	buf->st_atime,buf->st_mtime, buf->st_ctime);
  return 0;
}


/*********************************************************************
 *                  _tell           (CRTDLL.302)
 *
 * Get current file position.
 */
LONG __cdecl CRTDLL__tell(INT fd)
{
  return CRTDLL__lseek(fd, 0, SEEK_CUR);
}


/*********************************************************************
 *                  _tempnam           (CRTDLL.305)
 * 
 */
LPSTR __cdecl CRTDLL__tempnam(LPCSTR dir, LPCSTR prefix)
{
  char tmpbuf[MAX_PATH];

  TRACE("dir (%s) prefix (%s)\n",dir,prefix);
  if (GetTempFileNameA(dir,prefix,0,tmpbuf))
  {
    TRACE("got name (%s)\n",tmpbuf);
    return CRTDLL__strdup(tmpbuf);
  }
  TRACE("failed-last error (%ld)\n",GetLastError());
  return NULL;
}


/*********************************************************************
 *                  _umask           (CRTDLL.310)
 *
 * Set the process-wide umask.
 */
INT __cdecl CRTDLL__umask(INT umask)
{
  INT old_umask = __CRTDLL_umask;
  TRACE("umask (%d)\n",umask);
  __CRTDLL_umask = umask;
  return old_umask;
}


/*********************************************************************
 *                  _utime         (CRTDLL.314)
 * 
 * Set the file access/modification times on a file.
 */
INT __cdecl CRTDLL__utime(LPCSTR path, struct _utimbuf *t)
{
  INT fd = CRTDLL__open( path, _O_WRONLY | _O_BINARY );

  if (fd > 0)
  {
    INT retVal = CRTDLL__futime(fd, t);
    CRTDLL__close(fd);
    return retVal;
  }
  return -1;
}


/*********************************************************************
 *                  _unlink           (CRTDLL.315)
 *
 * Delete a file.
 */
INT __cdecl CRTDLL__unlink(LPCSTR path)
{
  TRACE("path (%s)\n",path);
  if(DeleteFileA( path ))
    return 0;

  TRACE("failed-last error (%ld)\n",GetLastError());
  __CRTDLL__set_errno(GetLastError());
  return -1;
}


/*********************************************************************
 *                  _write        (CRTDLL.332)
 *
 * Write data to a file.
 */
UINT __cdecl CRTDLL__write(INT fd, LPCVOID buf, UINT count)
{
  DWORD num_written;
  HANDLE hand = __CRTDLL__fdtoh(fd);

  /* Dont trace small writes, it gets *very* annoying */
  if (count > 4)
    TRACE(":fd (%d) handle (%d) buf (%p) len (%d)\n",fd,hand,buf,count);
  if (hand == INVALID_HANDLE_VALUE)
    return -1;

  /* If appending, go to EOF */
  if (__CRTDLL_flags[fd] & _IOAPPEND)
    CRTDLL__lseek(fd, 0, FILE_END );

  /* Set _cnt to 0 so optimised binaries will call our implementation
   * of putc/getc. See _filbuf/_flsbuf comments.
   */
  if (__CRTDLL_files[fd])
    __CRTDLL_files[fd]->_cnt = 0;

  if (WriteFile(hand, buf, count, &num_written, NULL)
      &&  (num_written == count))
    return num_written;

  TRACE(":failed-last error (%ld)\n",GetLastError());
  if ( __CRTDLL_files[fd])
     __CRTDLL_files[fd]->_flag |= _IOERR;

  return -1;
}


/*********************************************************************
 *                  clearerr     (CRTDLL.349)
 *
 * Clear a FILE's error indicator.
 */
VOID __cdecl CRTDLL_clearerr(CRTDLL_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);
  file->_flag &= ~(_IOERR | _IOEOF);
}


/*********************************************************************
 *                  fclose           (CRTDLL.362)
 *
 * Close an open file.
 */
INT __cdecl CRTDLL_fclose( CRTDLL_FILE* file )
{
  return CRTDLL__close(file->_file);
}


/*********************************************************************
 *                  feof           (CRTDLL.363)
 *
 * Check the eof indicator on a file.
 */
INT __cdecl CRTDLL_feof( CRTDLL_FILE* file )
{
  return file->_flag & _IOEOF;
}


/*********************************************************************
 *                  ferror         (CRTDLL.361)
 *
 * Check the error indicator on a file.
 */
INT __cdecl CRTDLL_ferror( CRTDLL_FILE* file )
{
  return file->_flag & _IOERR;
}


/*********************************************************************
 *                  fflush        (CRTDLL.362)
 */
INT __cdecl CRTDLL_fflush( CRTDLL_FILE* file )
{
  return CRTDLL__commit(file->_file);
}


/*********************************************************************
 *                  fgetc       (CRTDLL.363)
 */
INT __cdecl CRTDLL_fgetc( CRTDLL_FILE* file )
{
  char c;
  if (CRTDLL__read(file->_file,&c,1) != 1)
    return EOF;
  return c;
}


/*********************************************************************
 *                  fgetpos       (CRTDLL.364)
 */
INT __cdecl CRTDLL_fgetpos( CRTDLL_FILE* file, fpos_t *pos)
{
  *pos = CRTDLL__tell(file->_file);
  return (*pos == -1? -1 : 0);
}


/*********************************************************************
 *                  fgets       (CRTDLL.365)
 */
CHAR* __cdecl CRTDLL_fgets(LPSTR s, INT size, CRTDLL_FILE* file)
{
  int    cc;
  LPSTR  buf_start = s;

  TRACE(":file(%p) fd (%d) str (%p) len (%d)\n",
	file,file->_file,s,size);

  /* BAD, for the whole WINE process blocks... just done this way to test
   * windows95's ftp.exe.
   * JG - Is this true now we use ReadFile() on stdin too?
   */
  for(cc = CRTDLL_fgetc(file); cc != EOF && cc != '\n';
      cc = CRTDLL_fgetc(file))
    if (cc != '\r')
    {
      if (--size <= 0) break;
      *s++ = (char)cc;
    }
  if ((cc == EOF) && (s == buf_start)) /* If nothing read, return 0*/
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
 *                  fputs       (CRTDLL.375)
 */
INT __cdecl CRTDLL_fputs( LPCSTR s, CRTDLL_FILE* file )
{
  return CRTDLL_fwrite(s,strlen(s),1,file);
}


/*********************************************************************
 *                  fprintf       (CRTDLL.370)
 */
INT __cdecl CRTDLL_fprintf( CRTDLL_FILE* file, LPCSTR format, ... )
{
    va_list valist;
    INT res;

    va_start( valist, format );
    res = CRTDLL_vfprintf( file, format, valist );
    va_end( valist );
    return res;
}


/*********************************************************************
 *                  fopen     (CRTDLL.372)
 *
 * Open a file.
 */
CRTDLL_FILE* __cdecl CRTDLL_fopen(LPCSTR path, LPCSTR mode)
{
  CRTDLL_FILE* file;
  INT flags = 0, plus = 0, fd;
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

  fd = CRTDLL__open(path, flags);

  if (fd < 0)
    return NULL;

  file = __CRTDLL__alloc_fp(fd);
  TRACE(":get file (%p)\n",file);
  if (!file)
    CRTDLL__close(fd);

  return file;
}


/*********************************************************************
 *                  fputc       (CRTDLL.374)
 */
INT __cdecl CRTDLL_fputc( INT c, CRTDLL_FILE* file )
{
  return CRTDLL__write(file->_file, &c, 1) == 1? c : EOF;
}


/*********************************************************************
 *                  fread     (CRTDLL.377)
 */
DWORD __cdecl CRTDLL_fread(LPVOID ptr, INT size, INT nmemb, CRTDLL_FILE* file)
{
  DWORD read = CRTDLL__read(file->_file,ptr, size * nmemb);
  if (read <= 0)
    return 0;
  return read / size;
}


/*********************************************************************
 *                  freopen    (CRTDLL.379)
 * 
 */
CRTDLL_FILE* __cdecl CRTDLL_freopen(LPCSTR path, LPCSTR mode,CRTDLL_FILE* file)
{
  CRTDLL_FILE* newfile;
  INT fd;

  TRACE(":path (%p) mode (%s) file (%p) fd (%d)\n",path,mode,file,file->_file);
  if (!file || ((fd = file->_file) < 0) || fd > __CRTDLL_fdend)
    return NULL;

  if (fd > 2)
  {
    FIXME(":reopen on user file not implemented!\n");
    __CRTDLL__set_errno(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
  }

  /* first, create the new file */
  if ((newfile = CRTDLL_fopen(path,mode)) == NULL)
    return NULL;

  if (fd < 3 && SetStdHandle(fd == 0 ? STD_INPUT_HANDLE :
     (fd == 1? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE),
      __CRTDLL_handles[newfile->_file]))
  {
    /* Redirecting std handle to file , copy over.. */
    __CRTDLL_handles[fd] = __CRTDLL_handles[newfile->_file];
    __CRTDLL_flags[fd] = __CRTDLL_flags[newfile->_file];
    memcpy(&__CRTDLL_iob[fd], newfile, sizeof (CRTDLL_FILE));
    __CRTDLL_iob[fd]._file = fd;
    /* And free up the resources allocated by fopen, but
     * not the HANDLE we copied. */
    CRTDLL_free(__CRTDLL_files[fd]);
    __CRTDLL__free_fd(newfile->_file);
    return &__CRTDLL_iob[fd];
  }

  WARN(":failed-last error (%ld)\n",GetLastError());
  CRTDLL_fclose(newfile);
  __CRTDLL__set_errno(GetLastError());
  return NULL;
}


/*********************************************************************
 *                  fsetpos       (CRTDLL.380)
 */
INT __cdecl CRTDLL_fsetpos( CRTDLL_FILE* file, fpos_t *pos)
{
  return CRTDLL__lseek(file->_file,*pos,SEEK_SET);
}


/*********************************************************************
 *                  fscanf     (CRTDLL.381)
 */
INT __cdecl CRTDLL_fscanf( CRTDLL_FILE* file, LPSTR format, ... )
{
    INT rd = 0;
    int nch;
    va_list ap;
    if (!*format) return 0;
    WARN("%p (\"%s\"): semi-stub\n", file, format);
    nch = CRTDLL_fgetc(file);
    va_start(ap, format);
    while (*format) {
        if (*format == ' ') {
            /* skip whitespace */
            while ((nch!=EOF) && isspace(nch))
                nch = CRTDLL_fgetc(file);
        }
        else if (*format == '%') {
            int st = 0;
            format++;
            switch(*format) {
            case 'd': { /* read an integer */
                    int*val = va_arg(ap, int*);
                    int cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=EOF) && isspace(nch))
                        nch = CRTDLL_fgetc(file);
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = CRTDLL_fgetc(file);
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    nch = CRTDLL_fgetc(file);
                    /* read until no more digits */
                    while ((nch!=EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = CRTDLL_fgetc(file);
                    }
                    st = 1;
                    *val = cur;
                }
                break;
            case 'f': { /* read a float */
                    float*val = va_arg(ap, float*);
                    float cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=EOF) && isspace(nch))
                        nch = CRTDLL_fgetc(file);
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = CRTDLL_fgetc(file);
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    /* read until no more digits */
                    while ((nch!=EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = CRTDLL_fgetc(file);
                    }
                    if (nch == '.') {
                        /* handle decimals */
                        float dec = 1;
                        nch = CRTDLL_fgetc(file);
                        while ((nch!=EOF) && isdigit(nch)) {
                            dec /= 10;
                            cur += dec * (nch - '0');
                            nch = CRTDLL_fgetc(file);
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
                    while ((nch!=EOF) && isspace(nch))
                        nch = CRTDLL_fgetc(file);
                    /* read until whitespace */
                    while ((nch!=EOF) && !isspace(nch)) {
                        *sptr++ = nch; st++;
                        nch = CRTDLL_fgetc(file);
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
               nch = CRTDLL_fgetc(file);
            else break;
        }
        format++;
    }
    va_end(ap);
    if (nch!=EOF) {
        WARN("need ungetch\n");
    }
    TRACE("returning %d\n", rd);
    return rd;
}


/*********************************************************************
 *                  fseek     (CRTDLL.382)
 */
LONG __cdecl CRTDLL_fseek( CRTDLL_FILE* file, LONG offset, INT whence)
{
  return CRTDLL__lseek(file->_file,offset,whence);
}


/*********************************************************************
 *                  ftell     (CRTDLL.381)
 */
LONG __cdecl CRTDLL_ftell( CRTDLL_FILE* file )
{
  return CRTDLL__tell(file->_file);
}


/*********************************************************************
 *                  fwrite     (CRTDLL.383)
 */
UINT __cdecl CRTDLL_fwrite( LPCVOID ptr, INT size, INT nmemb, CRTDLL_FILE* file )
{
  UINT written = CRTDLL__write(file->_file, ptr, size * nmemb);
  if (written <= 0)
    return 0;
  return written / size;
}


/*********************************************************************
 *                  getchar       (CRTDLL.386)
 */
INT __cdecl CRTDLL_getchar( VOID )
{
  return CRTDLL_fgetc(CRTDLL_stdin);
}


/*********************************************************************
 *                  getc       (CRTDLL.388)
 */
INT __cdecl CRTDLL_getc( CRTDLL_FILE* file )
{
    return CRTDLL_fgetc( file );
}


/*********************************************************************
 *                  gets          (CRTDLL.391)
 */
LPSTR __cdecl CRTDLL_gets(LPSTR buf)
{
    int    cc;
    LPSTR  buf_start = buf;

    /* BAD, for the whole WINE process blocks... just done this way to test
     * windows95's ftp.exe.
     * JG 19/9/00: Is this still true, now we are using ReadFile?
     */
    for(cc = CRTDLL_fgetc(CRTDLL_stdin); cc != EOF && cc != '\n';
	cc = CRTDLL_fgetc(CRTDLL_stdin))
	if(cc != '\r') *buf++ = (char)cc;

    *buf = '\0';

    TRACE("got '%s'\n", buf_start);
    return buf_start;
}


/*********************************************************************
 *                  putc       (CRTDLL.441)
 */
INT __cdecl CRTDLL_putc( INT c, CRTDLL_FILE* file )
{
  return CRTDLL_fputc( c, file );
}


/*********************************************************************
 *                  putchar       (CRTDLL.442)
 */
void __cdecl CRTDLL_putchar( INT c )
{
  CRTDLL_fputc(c, CRTDLL_stdout);
}


/*********************************************************************
 *                  puts       (CRTDLL.443)
 */
INT __cdecl CRTDLL_puts(LPCSTR s)
{
  return CRTDLL_fputs(s, CRTDLL_stdout);
}


/*********************************************************************
 *                  rewind     (CRTDLL.447)
 *
 * Set the file pointer to the start of a file and clear any error
 * indicators.
 */
VOID __cdecl CRTDLL_rewind(CRTDLL_FILE* file)
{
  TRACE(":file (%p) fd (%d)\n",file,file->_file);
  CRTDLL__lseek(file->_file,0,SEEK_SET);
  file->_flag &= ~(_IOEOF | _IOERR);
}


/*********************************************************************
 *                  remove           (CRTDLL.448)
 */
INT __cdecl CRTDLL_remove(LPCSTR path)
{
  TRACE(":path (%s)\n",path);
  if (DeleteFileA(path))
    return 0;
  TRACE(":failed-last error (%ld)\n",GetLastError());
  __CRTDLL__set_errno(GetLastError());
  return -1;
}


/*********************************************************************
 *                  rename           (CRTDLL.449)
 */
INT __cdecl CRTDLL_rename(LPCSTR oldpath,LPCSTR newpath)
{
  TRACE(":from %s to %s\n",oldpath,newpath);
  if (MoveFileExA( oldpath, newpath, MOVEFILE_REPLACE_EXISTING))
    return 0;
  TRACE(":failed-last error (%ld)\n",GetLastError());
  __CRTDLL__set_errno(GetLastError());
  return -1;
}


/*********************************************************************
 *                  setbuf     (CRTDLL.452)
 */
INT __cdecl CRTDLL_setbuf(CRTDLL_FILE* file, LPSTR buf)
{
  TRACE(":file (%p) fd (%d) buf (%p)\n", file, file->_file,buf);
  if (buf)
    WARN(":user buffer will not be used!\n");
  /* FIXME: no buffering for now */
  return 0;
}


/*********************************************************************
 *                  tmpnam           (CRTDLL.490)
 *
 * lcclnk from lcc-win32 relies on a terminating dot in the name returned
 * 
 */
LPSTR __cdecl CRTDLL_tmpnam(LPSTR s)
{
  char tmpbuf[MAX_PATH];
  char* prefix = "TMP";
  if (!GetTempPathA(MAX_PATH,tmpbuf) ||
      !GetTempFileNameA(tmpbuf,prefix,0,CRTDLL_tmpname))
  {
    TRACE(":failed-last error (%ld)\n",GetLastError());
    return NULL;
  }
  TRACE(":got tmpnam %s\n",CRTDLL_tmpname);
  s = CRTDLL_tmpname;
  return s;
}


/*********************************************************************
 *                  vfprintf       (CRTDLL.494)
 *
 * Write formatted output to a file.
 */
/* we have avoided libc stdio.h so far, lets not start now */
extern int vsprintf(void *, const void *, va_list);

INT __cdecl CRTDLL_vfprintf( CRTDLL_FILE* file, LPCSTR format, va_list args )
{
  /* FIXME: We should parse the format string, calculate the maximum,
   * length of each arg, malloc a buffer, print to it, and fwrite that.
   * Yes this sucks, but not as much as crashing 1/2 way through an
   * app writing to a file :-(
   */
  char buffer[2048];
  TRACE(":file (%p) fd (%d) fmt (%s)\n",file,file->_file,format);

  vsprintf( buffer, format, args );
  return CRTDLL_fwrite( buffer, 1, strlen(buffer), file );
}

