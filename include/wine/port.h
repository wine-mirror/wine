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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_PORT_H
#define __WINE_WINE_PORT_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#define _GNU_SOURCE  /* for pread/pwrite */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_DIRECT_H
# include <direct.h>
#endif
#ifdef HAVE_IO_H
# include <io.h>
#endif
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/****************************************************************
 * Type definitions
 */

#ifndef HAVE_MODE_T
typedef int mode_t;
#endif
#ifndef HAVE_OFF_T
typedef long off_t;
#endif
#ifndef HAVE_PID_T
typedef int pid_t;
#endif
#ifndef HAVE_SIZE_T
typedef unsigned int size_t;
#endif
#ifndef HAVE_SSIZE_T
typedef int ssize_t;
#endif

#if !defined(HAVE_GETNETBYADDR) && !defined(HAVE_GETNETBYNAME)
struct netent {
  char         *n_name;
  char        **n_aliases;
  int           n_addrtype;
  unsigned long n_net;
};
#endif /* !defined(HAVE_GETNETBYADDR) && !defined(HAVE_GETNETBYNAME) */

#if !defined(HAVE_GETPROTOBYNAME) && !defined(HAVE_GETPROTOBYNUMBER)
struct  protoent {
  char  *p_name;
  char **p_aliases;
  int    p_proto;
};
#endif /* !defined(HAVE_GETPROTOBYNAME) && !defined(HAVE_GETPROTOBYNUMBER) */

#ifndef HAVE_STATFS
# ifdef __BEOS__
#  define STATFS_HAS_BFREE
struct statfs {
  long   f_bsize;  /* block_size */
  long   f_blocks; /* total_blocks */
  long   f_bfree;  /* free_blocks */
};
# else /* defined(__BEOS__) */
struct statfs;
# endif /* defined(__BEOS__) */
#endif /* !defined(HAVE_STATFS) */


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

#if !defined(HAVE_FTRUNCATE) && defined(HAVE_CHSIZE)
#define ftruncate chsize
#endif

#if !defined(HAVE_POPEN) && defined(HAVE__POPEN)
#define popen _popen
#endif

#if !defined(HAVE_PCLOSE) && defined(HAVE__PCLOSE)
#define pclose _pclose
#endif

#if !defined(HAVE_SNPRINTF) && defined(HAVE__SNPRINTF)
#define snprintf _snprintf
#endif

#ifndef S_ISLNK
# define S_ISLNK(mod) (0)
#endif /* S_ISLNK */

/* So we open files in 64 bit access mode on Linux */
#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif

/* Macros to define assembler functions somewhat portably */

#ifdef NEED_UNDERSCORE_PREFIX
# define __ASM_NAME(name) "_" name
#else
# define __ASM_NAME(name) name
#endif

#ifdef NEED_TYPE_IN_DEF
# define __ASM_FUNC(name) ".def " __ASM_NAME(name) "; .scl 2; .type 32; .endef"
#else
# define __ASM_FUNC(name) ".type " __ASM_NAME(name) ",@function"
#endif

#ifdef __GNUC__
# define __ASM_GLOBAL_FUNC(name,code) \
      __asm__( ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code );
#else  /* __GNUC__ */
# define __ASM_GLOBAL_FUNC(name,code) \
      void __asm_dummy_##name(void) { \
          asm( ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code ); \
      }
#endif  /* __GNUC__ */


/* Macros to access unaligned or wrong-endian WORDs and DWORDs. */

#define PUT_WORD(ptr, w)  (*(WORD *)(ptr) = (w))
#define GET_WORD(ptr)     (*(WORD *)(ptr))
#define PUT_DWORD(ptr, d) (*(DWORD *)(ptr) = (d))
#define GET_DWORD(ptr)    (*(DWORD *)(ptr))

#define PUT_LE_WORD(ptr, w) \
        do { ((BYTE *)(ptr))[0] = LOBYTE(w); \
             ((BYTE *)(ptr))[1] = HIBYTE(w); } while (0)
#define GET_LE_WORD(ptr) \
        MAKEWORD( ((BYTE *)(ptr))[0], \
                  ((BYTE *)(ptr))[1] )
#define PUT_LE_DWORD(ptr, d) \
        do { PUT_LE_WORD(&((WORD *)(ptr))[0], LOWORD(d)); \
             PUT_LE_WORD(&((WORD *)(ptr))[1], HIWORD(d)); } while (0)
#define GET_LE_DWORD(ptr) \
        ((DWORD)MAKELONG( GET_LE_WORD(&((WORD *)(ptr))[0]), \
                          GET_LE_WORD(&((WORD *)(ptr))[1]) ))

#define PUT_BE_WORD(ptr, w) \
        do { ((BYTE *)(ptr))[1] = LOBYTE(w); \
             ((BYTE *)(ptr))[0] = HIBYTE(w); } while (0)
#define GET_BE_WORD(ptr) \
        MAKEWORD( ((BYTE *)(ptr))[1], \
                  ((BYTE *)(ptr))[0] )
#define PUT_BE_DWORD(ptr, d) \
        do { PUT_BE_WORD(&((WORD *)(ptr))[1], LOWORD(d)); \
             PUT_BE_WORD(&((WORD *)(ptr))[0], HIWORD(d)); } while (0)
#define GET_BE_DWORD(ptr) \
        ((DWORD)MAKELONG( GET_BE_WORD(&((WORD *)(ptr))[1]), \
                          GET_BE_WORD(&((WORD *)(ptr))[0]) ))

#if defined(ALLOW_UNALIGNED_ACCESS)
#define PUT_UA_WORD(ptr, w)  PUT_WORD(ptr, w)
#define GET_UA_WORD(ptr)     GET_WORD(ptr)
#define PUT_UA_DWORD(ptr, d) PUT_DWORD(ptr, d)
#define GET_UA_DWORD(ptr)    GET_DWORD(ptr)
#elif defined(WORDS_BIGENDIAN)
#define PUT_UA_WORD(ptr, w)  PUT_BE_WORD(ptr, w)
#define GET_UA_WORD(ptr)     GET_BE_WORD(ptr)
#define PUT_UA_DWORD(ptr, d) PUT_BE_DWORD(ptr, d)
#define GET_UA_DWORD(ptr)    GET_BE_DWORD(ptr)
#else
#define PUT_UA_WORD(ptr, w)  PUT_LE_WORD(ptr, w)
#define GET_UA_WORD(ptr)     GET_LE_WORD(ptr)
#define PUT_UA_DWORD(ptr, d) PUT_LE_DWORD(ptr, d)
#define GET_UA_DWORD(ptr)    GET_LE_DWORD(ptr)
#endif


/****************************************************************
 * Function definitions (only when using libwine)
 */

#ifndef NO_LIBWINE

#if !defined(HAVE_CLONE) && defined(linux)
int clone(int (*fn)(void *arg), void *stack, int flags, void *arg);
#endif /* !defined(HAVE_CLONE) && defined(linux) */

#ifndef HAVE_GETNETBYADDR
struct netent *getnetbyaddr(unsigned long net, int type);
#endif /* defined(HAVE_GETNETBYNAME) */

#ifndef HAVE_GETNETBYNAME
struct netent *getnetbyname(const char *name);
#endif /* defined(HAVE_GETNETBYNAME) */

#ifndef HAVE_GETPAGESIZE
size_t getpagesize(void);
#endif  /* HAVE_GETPAGESIZE */

#ifndef HAVE_GETPROTOBYNAME
struct protoent *getprotobyname(const char *name);
#endif /* !defined(HAVE_GETPROTOBYNAME) */

#ifndef HAVE_GETPROTOBYNUMBER
struct protoent *getprotobynumber(int proto);
#endif /* !defined(HAVE_GETPROTOBYNUMBER) */

#ifndef HAVE_GETSERVBYPORT
struct servent *getservbyport(int port, const char *proto);
#endif /* !defined(HAVE_GETSERVBYPORT) */

#ifndef HAVE_GETSOCKOPT
int getsockopt(int socket, int level, int option_name, void *option_value, size_t *option_len);
#endif /* !defined(HAVE_GETSOCKOPT) */

#ifndef HAVE_INET_NETWORK
unsigned long inet_network(const char *cp);
#endif /* !defined(HAVE_INET_NETWORK) */

#ifndef HAVE_LSTAT
int lstat(const char *file_name, struct stat *buf);
#endif /* HAVE_LSTAT */

#ifndef HAVE_MEMMOVE
void *memmove(void *dest, const void *src, unsigned int len);
#endif /* !defined(HAVE_MEMMOVE) */

#ifndef HAVE_PREAD
ssize_t pread( int fd, void *buf, size_t count, off_t offset );
#endif /* HAVE_PREAD */

#ifndef HAVE_PWRITE
ssize_t pwrite( int fd, const void *buf, size_t count, off_t offset );
#endif /* HAVE_PWRITE */

#ifndef HAVE_STATFS
int statfs(const char *name, struct statfs *info);
#endif /* !defined(HAVE_STATFS) */

#ifndef HAVE_STRNCASECMP
# ifndef HAVE__STRNICMP
int strncasecmp(const char *str1, const char *str2, size_t n);
# else
# define strncasecmp _strnicmp
# endif
#endif /* !defined(HAVE_STRNCASECMP) */

#ifndef HAVE_STRERROR
const char *strerror(int err);
#endif /* !defined(HAVE_STRERROR) */

#ifndef HAVE_STRCASECMP
# ifndef HAVE__STRICMP
int strcasecmp(const char *str1, const char *str2);
# else
# define strcasecmp _stricmp
# endif
#endif /* !defined(HAVE_STRCASECMP) */

#ifndef HAVE_USLEEP
int usleep (unsigned int useconds);
#endif /* !defined(HAVE_USLEEP) */

/* Interlocked functions */

#if defined(__i386__) && defined(__GNUC__)

inline static long interlocked_cmpxchg( long *dest, long xchg, long compare )
{
    long ret;
    __asm__ __volatile__( "lock; cmpxchgl %2,(%1)"
                          : "=a" (ret) : "r" (dest), "r" (xchg), "0" (compare) : "memory" );
    return ret;
}

inline static void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare )
{
    void *ret;
    __asm__ __volatile__( "lock; cmpxchgl %2,(%1)"
                          : "=a" (ret) : "r" (dest), "r" (xchg), "0" (compare) : "memory" );
    return ret;
}

inline static long interlocked_xchg( long *dest, long val )
{
    long ret;
    __asm__ __volatile__( "lock; xchgl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (val) : "memory" );
    return ret;
}

inline static void *interlocked_xchg_ptr( void **dest, void *val )
{
    void *ret;
    __asm__ __volatile__( "lock; xchgl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (val) : "memory" );
    return ret;
}

inline static long interlocked_xchg_add( long *dest, long incr )
{
    long ret;
    __asm__ __volatile__( "lock; xaddl %0,(%1)"
                          : "=r" (ret) : "r" (dest), "0" (incr) : "memory" );
    return ret;
}

#else  /* __i386___ && __GNUC__ */

extern long interlocked_cmpxchg( long *dest, long xchg, long compare );
extern void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare );
extern long interlocked_xchg( long *dest, long val );
extern void *interlocked_xchg_ptr( void **dest, void *val );
extern long interlocked_xchg_add( long *dest, long incr );

#endif  /* __i386___ && __GNUC__ */

#else /* NO_LIBWINE */

#define __WINE_NOT_PORTABLE(func) func##_is_not_portable func##_is_not_portable

#define clone             __WINE_NOT_PORTABLE(clone)
#define getnetbyaddr      __WINE_NOT_PORTABLE(getnetbyaddr)
#define getnetbyname      __WINE_NOT_PORTABLE(getnetbyname)
#define getpagesize       __WINE_NOT_PORTABLE(getpagesize)
#define getprotobyname    __WINE_NOT_PORTABLE(getprotobyname)
#define getprotobynumber  __WINE_NOT_PORTABLE(getprotobynumber)
#define getservbyport     __WINE_NOT_PORTABLE(getservbyport)
#define getsockopt        __WINE_NOT_PORTABLE(getsockopt)
#define inet_network      __WINE_NOT_PORTABLE(inet_network)
#define lstat             __WINE_NOT_PORTABLE(lstat)
#define memmove           __WINE_NOT_PORTABLE(memmove)
#define pread             __WINE_NOT_PORTABLE(pread)
#define pwrite            __WINE_NOT_PORTABLE(pwrite)
#define statfs            __WINE_NOT_PORTABLE(statfs)
#define strcasecmp        __WINE_NOT_PORTABLE(strcasecmp)
#define strerror          __WINE_NOT_PORTABLE(strerror)
#define strncasecmp       __WINE_NOT_PORTABLE(strncasecmp)
#define usleep            __WINE_NOT_PORTABLE(usleep)

#endif /* NO_LIBWINE */

#endif /* !defined(__WINE_WINE_PORT_H) */
