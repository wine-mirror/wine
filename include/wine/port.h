/*
 * Wine porting definitions
 *
 * Copyright 1996 Alexandre Julliard
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
 */

#ifndef __WINE_WINE_PORT_H
#define __WINE_WINE_PORT_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef __WINE_BASETSD_H
# error You must include port.h before all other headers
#endif

#ifndef _GNU_SOURCE
# define _GNU_SOURCE  /* for pread/pwrite, isfinite */
#endif
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/****************************************************************
 * Hard-coded values for the Windows platform
 */

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <direct.h>
#include <io.h>
#include <process.h>

#define mkdir(path,mode) mkdir(path)

static inline void *dlopen(const char *name, int flags) { return NULL; }
static inline void *dlsym(void *handle, const char *name) { return NULL; }
static inline int dlclose(void *handle) { return 0; }
static inline const char *dlerror(void) { return "No dlopen support on Windows"; }

#ifdef _MSC_VER

#define popen _popen
#define pclose _pclose
/* The UCRT headers in the Windows SDK #error out if we #define snprintf.
 * The C headers that came with previous Visual Studio versions do not have
 * snprintf. Check for VS 2015, which appears to be the first version to
 * use the UCRT headers by default. */
#if _MSC_VER < 1900
# define snprintf _snprintf
#endif
#define strtoll _strtoi64
#define strtoull _strtoui64
#define strncasecmp _strnicmp
#define strcasecmp _stricmp

typedef long off_t;
typedef int pid_t;
typedef int ssize_t;

#endif /* _MSC_VER */

#else  /* _WIN32 */

#ifndef __int64
#  if defined(__x86_64__) || defined(__aarch64__) || defined(_WIN64)
#    define __int64 long
#  else
#    define __int64 long long
#  endif
#endif

/* Process creation flags */
#ifndef _P_WAIT
# define _P_WAIT    0
# define _P_NOWAIT  1
# define _P_OVERLAY 2
# define _P_NOWAITO 3
# define _P_DETACH  4
#endif
#ifndef HAVE__SPAWNVP
extern int _spawnvp(int mode, const char *cmdname, const char * const argv[]);
#endif

#endif  /* _WIN32 */

/****************************************************************
 * Macro definitions
 */

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#define RTLD_LAZY    0x001
#define RTLD_NOW     0x002
#define RTLD_GLOBAL  0x100
#endif

#ifndef S_ISLNK
# define S_ISLNK(mod) (0)
#endif

#ifndef S_ISDIR
# define S_ISDIR(mod) (((mod) & _S_IFMT) == _S_IFDIR)
#endif

#ifndef S_ISCHR
# define S_ISCHR(mod) (((mod) & _S_IFMT) == _S_IFCHR)
#endif

#ifndef S_ISREG
# define S_ISREG(mod) (((mod) & _S_IFMT) == _S_IFREG)
#endif

/* So we open files in 64 bit access mode on Linux */
#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif


/****************************************************************
 * Constants
 */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.570796326794896619
#endif


/****************************************************************
 * Function definitions (only when using libwine_port)
 */

#ifndef HAVE_GETOPT_LONG_ONLY
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;
struct option;

#ifndef HAVE_STRUCT_OPTION_NAME
struct option
{
    const char *name;
    int has_arg;
    int *flag;
    int val;
};
#endif

extern int getopt_long (int ___argc, char *const *___argv,
                        const char *__shortopts,
                        const struct option *__longopts, int *__longind);
extern int getopt_long_only (int ___argc, char *const *___argv,
                             const char *__shortopts,
                             const struct option *__longopts, int *__longind);
#endif  /* HAVE_GETOPT_LONG_ONLY */

#ifndef HAVE_LSTAT
int lstat(const char *file_name, struct stat *buf);
#endif /* HAVE_LSTAT */

#ifndef HAVE_POLL
struct pollfd
{
    int fd;
    short events;
    short revents;
};
#define POLLIN   0x01
#define POLLPRI  0x02
#define POLLOUT  0x04
#define POLLERR  0x08
#define POLLHUP  0x10
#define POLLNVAL 0x20
int poll( struct pollfd *fds, unsigned int count, int timeout );
#endif /* HAVE_POLL */

#ifndef HAVE_PREAD
ssize_t pread( int fd, void *buf, size_t count, off_t offset );
#endif /* HAVE_PREAD */

#ifndef HAVE_PWRITE
ssize_t pwrite( int fd, const void *buf, size_t count, off_t offset );
#endif /* HAVE_PWRITE */

#ifndef HAVE_READLINK
int readlink( const char *path, char *buf, size_t size );
#endif /* HAVE_READLINK */

#ifndef HAVE_SYMLINK
int symlink(const char *from, const char *to);
#endif

extern int mkstemps(char *template, int suffix_len);

#endif /* !defined(__WINE_WINE_PORT_H) */
