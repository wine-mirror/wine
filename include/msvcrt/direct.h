/*
 * Path and directory definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_DIRECT_H
#define __WINE_DIRECT_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <pshpack8.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

#if defined(__x86_64__) && !defined(_WIN64)
#define _WIN64
#endif

#if !defined(_MSC_VER) && !defined(__int64)
# ifdef _WIN64
#   define __int64 long
# else
#   define __int64 long long
# endif
#endif

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _DISKFREE_T_DEFINED
#define _DISKFREE_T_DEFINED
struct _diskfree_t {
  unsigned int total_clusters;
  unsigned int avail_clusters;
  unsigned int sectors_per_cluster;
  unsigned int bytes_per_sector;
};
#endif /* _DISKFREE_T_DEFINED */

int         _chdir(const char*);
int         _chdrive(int);
char*       _getcwd(char*,int);
char*       _getdcwd(int,char*,int);
int         _getdrive(void);
unsigned long _getdrives(void);
int         _mkdir(const char*);
int         _rmdir(const char*);

#ifndef _WDIRECT_DEFINED
#define _WDIRECT_DEFINED
int              _wchdir(const wchar_t*);
wchar_t* _wgetcwd(wchar_t*,int);
wchar_t* _wgetdcwd(int,wchar_t*,int);
int              _wmkdir(const wchar_t*);
int              _wrmdir(const wchar_t*);
#endif /* _WDIRECT_DEFINED */

#ifdef __cplusplus
}
#endif


static inline int chdir(const char* newdir) { return _chdir(newdir); }
static inline char* getcwd(char * buf, int size) { return _getcwd(buf, size); }
static inline int mkdir(const char* newdir) { return _mkdir(newdir); }
static inline int rmdir(const char* dir) { return _rmdir(dir); }

#include <poppack.h>

#endif /* __WINE_DIRECT_H */
