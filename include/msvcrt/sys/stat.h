/*
 * _stat() definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_SYS_STAT_H
#define __WINE_SYS_STAT_H
#define __WINE_USE_MSVCRT

#include "msvcrt/sys/types.h"


#define _S_IEXEC  0x0040
#define _S_IWRITE 0x0080
#define _S_IREAD  0x0100
#define _S_IFIFO  0x1000
#define _S_IFCHR  0x2000
#define _S_IFDIR  0x4000
#define _S_IFREG  0x8000
#define _S_IFMT   0xF000


struct _stat {
  _dev_t         st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t         st_rdev;
  _off_t         st_size;
  MSVCRT(time_t) st_atime;
  MSVCRT(time_t) st_mtime;
  MSVCRT(time_t) st_ctime;
};

struct _stati64 {
  _dev_t         st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t         st_rdev;
  __int64        st_size;
  MSVCRT(time_t) st_atime;
  MSVCRT(time_t) st_mtime;
  MSVCRT(time_t) st_ctime;
};


#ifdef __cplusplus
extern "C" {
#endif

int _fstat(int,struct _stat*);
int _fstati64(int,struct _stati64*);
int _stat(const char*,struct _stat*);
int _stati64(const char*,struct _stati64*);

int _wstat(const WCHAR*,struct _stat*);
int _wstati64(const WCHAR*,struct _stati64*);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define S_IFMT   _S_IFMT
#define S_IFDIR  _S_IFDIR
#define S_IFCHR  _S_IFCHR
#define S_IFREG  _S_IFREG
#define S_IREAD  _S_IREAD
#define S_IWRITE _S_IWRITE
#define S_IEXEC  _S_IEXEC

#define fstat _fstat
#define stat _stat
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SYS_STAT_H */
