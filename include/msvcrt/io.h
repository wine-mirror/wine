/*
 * System I/O definitions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_IO_H
#define __WINE_IO_H
#define __WINE_USE_MSVCRT

#include "msvcrt/stdio.h"          /* For FILENAME_MAX */
#include "msvcrt/sys/types.h"      /* For time_t */

/* The following are also defined in dos.h */
#define _A_NORMAL 0x00000000
#define _A_RDONLY 0x00000001
#define _A_HIDDEN 0x00000002
#define _A_SYSTEM 0x00000004
#define _A_VOLID  0x00000008
#define _A_SUBDIR 0x00000010
#define _A_ARCH   0x00000020

typedef unsigned long _fsize_t;

struct _finddata_t
{
  unsigned attrib;
  MSVCRT(time_t) time_create;
  MSVCRT(time_t) time_access;
  MSVCRT(time_t) time_write;
  _fsize_t       size;
  char           name[MSVCRT(FILENAME_MAX)];
};

struct _finddatai64_t
{
  unsigned attrib;
  MSVCRT(time_t) time_create;
  MSVCRT(time_t) time_access;
  MSVCRT(time_t) time_write;
  __int64        size;
  char           name[MSVCRT(FILENAME_MAX)];
};

struct _wfinddata_t {
  unsigned attrib;
  MSVCRT(time_t) time_create;
  MSVCRT(time_t) time_access;
  MSVCRT(time_t) time_write;
  _fsize_t       size;
  WCHAR          name[MSVCRT(FILENAME_MAX)];
};

struct _wfinddatai64_t {
  unsigned attrib;
  MSVCRT(time_t) time_create;
  MSVCRT(time_t) time_access;
  MSVCRT(time_t) time_write;
  __int64        size;
  WCHAR          name[MSVCRT(FILENAME_MAX)];
};


#ifdef __cplusplus
extern "C" {
#endif

int         _access(const char*,int);
int         _chmod(const char*,int);
int         _chsize(int,long);
int         _close(int);
int         _commit(int);
int         _creat(const char*,int);
int         _dup(int);
int         _dup2(int,int);
int         _eof(int);
__int64     _filelengthi64(int);
long        _filelength(int);
int         _findclose(long);
long        _findfirst(const char*,struct _finddata_t*);
long        _findfirsti64(const char*, struct _finddatai64_t*);
int         _findnext(long,struct _finddata_t*);
int         _findnexti64(long, struct _finddatai64_t*);
long        _get_osfhandle(int);
int         _isatty(int);
int         _locking(int,int,long);
long        _lseek(int,long,int);
__int64     _lseeki64(int,__int64,int);
char*       _mktemp(char*);
int         _open(const char*,int,...);
int         _open_osfhandle(long,int);
int         _pipe(int*,unsigned int,int);
int         _read(int,void*,unsigned int);
int         _setmode(int,int);
int         _sopen(const char*,int,int,...);
long        _tell(int);
__int64     _telli64(int);
int         _umask(int);
int         _unlink(const char*);
int         _write(int,const void*,unsigned int);

int         MSVCRT(remove)(const char*);
int         MSVCRT(rename)(const char*,const char*);

int         _waccess(const WCHAR*,int);
int         _wchmod(const WCHAR*,int);
int         _wcreat(const WCHAR*,int);
long        _wfindfirst(const WCHAR*,struct _wfinddata_t*);
long        _wfindfirsti64(const WCHAR*, struct _wfinddatai64_t*);
int         _wfindnext(long,struct _wfinddata_t*);
int         _wfindnexti64(long, struct _wfinddatai64_t*);
WCHAR*      _wmktemp(WCHAR*);
int         _wopen(const WCHAR*,int,...);
int         _wrename(const WCHAR*,const WCHAR*);
int         _wsopen(const WCHAR*,int,int,...);
int         _wunlink(const WCHAR*);


#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define access _access
#define chmod _chmod
#define chsize _chsize
#define close _close
#define creat _creat
#define dup _dup
#define dup2 _dup2
#define eof _eof
#define filelength _filelength
#define isatty _isatty
#define locking _locking
#define lseek _lseek
#define mktemp _mktemp
#define open _open
#define read _read
#define setmode _setmode
#define sopen _sopen
#define tell _tell
#define umask _umask
#define unlink _unlink
#define write _write
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_IO_H */
