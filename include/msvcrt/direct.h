/*
 * Path and directory definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_DIRECT_H
#define __WINE_DIRECT_H
#define __WINE_USE_MSVCRT

#include "winnt.h"
#include "msvcrt/dos.h"            /* For _getdiskfree & co */


#ifdef __cplusplus
extern "C" {
#endif

int         _chdir(const char*);
int         _chdrive(int);
char*       _getcwd(char*,int);
char*       _getdcwd(int,char*,int);
int         _getdrive(void);
unsigned long _getdrives(void);
int         _mkdir(const char*);
int         _rmdir(const char*);

int         _wchdir(const WCHAR*);
WCHAR*      _wgetcwd(WCHAR*,int);
WCHAR*      _wgetdcwd(int,WCHAR*,int);
int         _wmkdir(const WCHAR*);
int         _wrmdir(const WCHAR*);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define chdir _chdir
#define getcwd _getcwd
#define mkdir _mkdir
#define rmdir _rmdir
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_DIRECT_H */
