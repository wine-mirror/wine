/*
 * Misc. functions for systems that don't have them
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

#include "config.h"
#include "wine/port.h"

#ifdef __BEOS__
#include <be/kernel/fs_info.h>
#include <be/kernel/OS.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_INTTYPES_H
# include <sys/inttypes.h>
#endif
#ifdef HAVE_SYS_TIME_h
# include <sys/time.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_LIBIO_H
# include <libio.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

/***********************************************************************
 *		usleep
 */
#ifndef HAVE_USLEEP
int usleep (unsigned int useconds)
{
#if defined(__EMX__)
    DosSleep(useconds);
    return 0;
#elif defined(__BEOS__)
    return snooze(useconds);
#elif defined(HAVE_SELECT)
    struct timeval delay;

    delay.tv_sec = useconds / 1000000;
    delay.tv_usec = useconds % 1000000;

    select( 0, 0, 0, 0, &delay );
    return 0;
#else /* defined(__EMX__) || defined(__BEOS__) || defined(HAVE_SELECT) */
    errno = ENOSYS;
    return -1;
#endif /* defined(__EMX__) || defined(__BEOS__) || defined(HAVE_SELECT) */
}
#endif /* HAVE_USLEEP */

/***********************************************************************
 *		memmove
 */
#ifndef HAVE_MEMMOVE
void *memmove( void *dest, const void *src, size_t len )
{
    register char *dst = dest;

    /* Use memcpy if not overlapping */
    if ((dst + len <= (char *)src) || ((char *)src + len <= dst))
    {
        memcpy( dst, src, len );
    }
    /* Otherwise do it the hard way (FIXME: could do better than this) */
    else if (dst < (char *)src)
    {
        while (len--) *dst++ = *((char *)src)++;
    }
    else
    {
        dst += len - 1;
        src = (char *)src + len - 1;
        while (len--) *dst-- = *((char *)src)--;
    }
    return dest;
}
#endif  /* HAVE_MEMMOVE */

/***********************************************************************
 *		strerror
 */
#ifndef HAVE_STRERROR
const char *strerror( int err )
{
    /* Let's hope we have sys_errlist then */
    return sys_errlist[err];
}
#endif  /* HAVE_STRERROR */


/***********************************************************************
 *		getpagesize
 */
#ifndef HAVE_GETPAGESIZE
size_t getpagesize(void)
{
# ifdef __svr4__
    return sysconf(_SC_PAGESIZE);
# elif defined(__i386__)
    return 4096;
# else
#  error Cannot get the page size on this platform
# endif
}
#endif  /* HAVE_GETPAGESIZE */


/***********************************************************************
 *		clone
 */
#if !defined(HAVE_CLONE) && defined(__linux__)
int clone( int (*fn)(void *), void *stack, int flags, void *arg )
{
#ifdef __i386__
    int ret;
    void **stack_ptr = (void **)stack;
    *--stack_ptr = arg;  /* Push argument on stack */
    *--stack_ptr = fn;   /* Push function pointer (popped into ebx) */
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %2,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx\n\t"   /* Contains fn in the child */
                          "testl %%eax,%%eax\n\t"
                          "jnz 0f\n\t"
                          "xorl %ebp,%ebp\n\t"    /* Terminate the stack frames */
                          "call *%%ebx\n\t"       /* Should never return */
                          "xorl %%eax,%%eax\n\t"  /* Just in case it does*/
                          "0:"
                          : "=a" (ret)
                          : "0" (SYS_clone), "r" (flags), "c" (stack_ptr) );
    assert( ret );  /* If ret is 0, we returned from the child function */
    if (ret > 0) return ret;
    errno = -ret;
    return -1;
#else
    errno = EINVAL;
    return -1;
#endif  /* __i386__ */
}
#endif  /* !HAVE_CLONE && __linux__ */

/***********************************************************************
 *		strcasecmp
 */
#ifndef HAVE_STRCASECMP
int strcasecmp( const char *str1, const char *str2 )
{
    const unsigned char *ustr1 = (const unsigned char *)str1;
    const unsigned char *ustr2 = (const unsigned char *)str2;

    while (*ustr1 && toupper(*ustr1) == toupper(*ustr2)) {
        ustr1++;
	ustr2++;
    }
    return toupper(*ustr1) - toupper(*ustr2);
}
#endif /* HAVE_STRCASECMP */

/***********************************************************************
 *		strncasecmp
 */
#ifndef HAVE_STRNCASECMP
int strncasecmp( const char *str1, const char *str2, size_t n )
{
    const unsigned char *ustr1 = (const unsigned char *)str1;
    const unsigned char *ustr2 = (const unsigned char *)str2;
    int res;

    if (!n) return 0;
    while ((--n > 0) && *ustr1) {
	if ((res = toupper(*ustr1) - toupper(*ustr2))) return res;
	ustr1++;
	ustr2++;
    }
    return toupper(*ustr1) - toupper(*ustr2);
}
#endif /* HAVE_STRNCASECMP */

/***********************************************************************
 *		getsockopt
 */
#ifndef HAVE_GETSOCKOPT
int getsockopt(int socket, int level, int option_name,
               void *option_value, size_t *option_len)
{
    errno = ENOSYS;
    return -1;
}
#endif /* !defined(HAVE_GETSOCKOPT) */

/***********************************************************************
 *		inet_network
 */
#ifndef HAVE_INET_NETWORK
unsigned long inet_network(const char *cp)
{
    errno = ENOSYS;
    return 0;
}
#endif /* defined(HAVE_INET_NETWORK) */

/***********************************************************************
 *		statfs
 */
#ifndef HAVE_STATFS
int statfs(const char *name, struct statfs *info)
{
#ifdef __BEOS__
    dev_t mydev;
    fs_info fsinfo;

    if(!info) {
        errno = ENOSYS;
        return -1;
    }

    if ((mydev = dev_for_path(name)) < 0) {
        errno = ENOSYS;
        return -1;
    }

    if (fs_stat_dev(mydev,&fsinfo) < 0) {
        errno = ENOSYS;
        return -1;
    }

    info->f_bsize = fsinfo.block_size;
    info->f_blocks = fsinfo.total_blocks;
    info->f_bfree = fsinfo.free_blocks;
    return 0;
#else /* defined(__BEOS__) */
    errno = ENOSYS;
    return -1;
#endif /* defined(__BEOS__) */
}
#endif /* !defined(HAVE_STATFS) */


/***********************************************************************
 *		lstat
 */
#ifndef HAVE_LSTAT
int lstat(const char *file_name, struct stat *buf)
{
    return stat( file_name, buf );
}
#endif /* HAVE_LSTAT */

/***********************************************************************
 *		mkstemp
 */
#ifndef HAVE_MKSTEMP
int mkstemp(char *tmpfn)
{
    int tries;
    char *xstart;

    xstart = tmpfn+strlen(tmpfn)-1;
    while ((xstart > tmpfn) && (*xstart == 'X'))
        xstart--;
    tries = 10;
    while (tries--) {
    	char *newfn = mktemp(tmpfn);
	int fd;
	if (!newfn) /* something else broke horribly */
	    return -1;
	fd = open(newfn,O_CREAT|O_RDWR|O_EXCL,0600);
	if (fd!=-1)
	    return fd;
	newfn = xstart;
	/* fill up with X and try again ... */
	while (*newfn) *newfn++ = 'X';
    }
    return -1;
}
#endif /* HAVE_MKSTEMP */


/***********************************************************************
 *		pread
 *
 * FIXME: this is not thread-safe
 */
#ifndef HAVE_PREAD
ssize_t pread( int fd, void *buf, size_t count, off_t offset )
{
    ssize_t ret;
    off_t old_pos;

    if ((old_pos = lseek( fd, 0, SEEK_CUR )) == -1) return -1;
    if (lseek( fd, offset, SEEK_SET ) == -1) return -1;
    if ((ret = read( fd, buf, count )) == -1)
    {
        int err = errno;  /* save errno */
        lseek( fd, old_pos, SEEK_SET );
        errno = err;
        return -1;
    }
    if (lseek( fd, old_pos, SEEK_SET ) == -1) return -1;
    return ret;
}
#endif /* HAVE_PREAD */


/***********************************************************************
 *		pwrite
 *
 * FIXME: this is not thread-safe
 */
#ifndef HAVE_PWRITE
ssize_t pwrite( int fd, const void *buf, size_t count, off_t offset )
{
    ssize_t ret;
    off_t old_pos;

    if ((old_pos = lseek( fd, 0, SEEK_CUR )) == -1) return -1;
    if (lseek( fd, offset, SEEK_SET ) == -1) return -1;
    if ((ret = write( fd, buf, count )) == -1)
    {
        int err = errno;  /* save errno */
        lseek( fd, old_pos, SEEK_SET );
        errno = err;
        return -1;
    }
    if (lseek( fd, old_pos, SEEK_SET ) == -1) return -1;
    return ret;
}
#endif /* HAVE_PWRITE */


#if defined(__svr4__) || defined(__NetBSD__)
/***********************************************************************
 *             try_mmap_fixed
 *
 * The purpose of this routine is to emulate the behaviour of
 * the Linux mmap() routine if a non-NULL address is passed,
 * but the MAP_FIXED flag is not set.  Linux in this case tries
 * to place the mapping at the specified address, *unless* the
 * range is already in use.  Solaris, however, completely ignores
 * the address argument in this case.
 *
 * As Wine code occasionally relies on the Linux behaviour, e.g. to
 * be able to map non-relocateable PE executables to their proper
 * start addresses, or to map the DOS memory to 0, this routine
 * emulates the Linux behaviour by checking whether the desired
 * address range is still available, and placing the mapping there
 * using MAP_FIXED if so.
 */
static int try_mmap_fixed (void *addr, size_t len, int prot, int flags,
                           int fildes, off_t off)
{
    char * volatile result = NULL;
    int pagesize = getpagesize();
    pid_t pid;

    /* We only try to map to a fixed address if
       addr is non-NULL and properly aligned,
       and MAP_FIXED isn't already specified. */

    if ( !addr )
        return 0;
    if ( (uintptr_t)addr & (pagesize-1) )
        return 0;
    if ( flags & MAP_FIXED )
        return 0;

    /* We use vfork() to freeze all threads of the
       current process.  This allows us to check without
       race condition whether the desired memory range is
       already in use.  Note that because vfork() shares
       the address spaces between parent and child, we
       can actually perform the mapping in the child. */

    if ( (pid = vfork()) == -1 )
    {
        perror("try_mmap_fixed: vfork");
        exit(1);
    }
    if ( pid == 0 )
    {
        int i;
        char vec;

        /* We call mincore() for every page in the desired range.
           If any of these calls succeeds, the page is already
           mapped and we must fail. */
        for ( i = 0; i < len; i += pagesize )
            if ( mincore( (caddr_t)addr + i, pagesize, &vec ) != -1 )
               _exit(1);

        /* Perform the mapping with MAP_FIXED set.  This is safe
           now, as none of the pages is currently in use. */
        result = mmap( addr, len, prot, flags | MAP_FIXED, fildes, off );
        if ( result == addr )
            _exit(0);

        if ( result != (void *) -1 ) /* This should never happen ... */
            munmap( result, len );

       _exit(1);
    }

    /* vfork() lets the parent continue only after the child
       has exited.  Furthermore, Wine sets SIGCHLD to SIG_IGN,
       so we don't need to wait for the child. */

    return result == addr;
}
#endif

/***********************************************************************
 *		wine_anon_mmap
 *
 * Portable wrapper for anonymous mmaps
 */
void *wine_anon_mmap( void *start, size_t size, int prot, int flags )
{
#ifdef HAVE_MMAP
    static int fdzero = -1;

#ifdef MAP_ANON
    flags |= MAP_ANON;
#else
    if (fdzero == -1)
    {
        if ((fdzero = open( "/dev/zero", O_RDONLY )) == -1)
        {
            perror( "/dev/zero: open" );
            exit(1);
        }
    }
#endif  /* MAP_ANON */

#ifdef MAP_SHARED
    flags &= ~MAP_SHARED;
#endif

    /* Linux EINVAL's on us if we don't pass MAP_PRIVATE to an anon mmap */
#ifdef MAP_PRIVATE
    flags |= MAP_PRIVATE;
#endif

#if defined(__svr4__) || defined(__NetBSD__)
    if ( try_mmap_fixed( start, size, prot, flags, fdzero, 0 ) )
        return start;
#endif

    return mmap( start, size, prot, flags, fdzero, 0 );
#else
    return (void *)-1;
#endif
}


/*
 * These functions provide wrappers around dlopen() and associated
 * functions.  They work around a bug in glibc 2.1.x where calling
 * a dl*() function after a previous dl*() function has failed
 * without a dlerror() call between the two will cause a crash.
 * They all take a pointer to a buffer that
 * will receive the error description (from dlerror()).  This
 * parameter may be NULL if the error description is not required.
 */

/***********************************************************************
 *		wine_dlopen
 */
void *wine_dlopen( const char *filename, int flag, char *error, int errorsize )
{
#ifdef HAVE_DLOPEN
    void *ret;
    const char *s;
    dlerror(); dlerror();
    ret = dlopen( filename, flag );
    s = dlerror();
    if (error)
    {
	strncpy( error, s ? s : "", errorsize );
	error[errorsize - 1] = '\0';
    }
    dlerror();
    return ret;
#else
    if (error)
    {
	strncpy( error, "dlopen interface not detected by configure", errorsize );
	error[errorsize - 1] = '\0';
    }
    return NULL;
#endif
}

/***********************************************************************
 *		wine_dlsym
 */
void *wine_dlsym( void *handle, const char *symbol, char *error, int errorsize )
{
#ifdef HAVE_DLOPEN
    void *ret;
    const char *s;
    dlerror(); dlerror();
    ret = dlsym( handle, symbol );
    s = dlerror();
    if (error)
    {
	strncpy( error, s ? s : "", errorsize );
	error[errorsize - 1] = '\0';
    }
    dlerror();
    return ret;
#else
    if (error)
    {
	strncpy( error, "dlopen interface not detected by configure", errorsize );
	error[errorsize - 1] = '\0';
    }
    return NULL;
#endif
}

/***********************************************************************
 *		wine_dlclose
 */
int wine_dlclose( void *handle, char *error, int errorsize )
{
#ifdef HAVE_DLOPEN
    int ret;
    const char *s;
    dlerror(); dlerror();
    ret = dlclose( handle );
    s = dlerror();
    if (error)
    {
	strncpy( error, s ? s : "", errorsize );
	error[errorsize - 1] = '\0';
    }
    dlerror();
    return ret;
#else
    if (error)
    {
	strncpy( error, "dlopen interface not detected by configure", errorsize );
	error[errorsize - 1] = '\0';
    }
    return 1;
#endif
}

/***********************************************************************
 *		wine_rewrite_s4tos2
 *
 * Convert 4 byte Unicode strings to 2 byte Unicode strings in-place.
 * This is only practical if literal strings are writable.
 */
unsigned short* wine_rewrite_s4tos2(const wchar_t* str4 )
{
    unsigned short *str2,*s2;

    if (str4==NULL)
      return NULL;

    if ((*str4 & 0xffff0000) != 0) {
        /* This string has already been converted. Return it as is */
        return (unsigned short*)str4;
    }

    /* Note that we can also end up here if the string has a single
     * character. In such a case we will convert the string over and
     * over again. But this is harmless.
     */
    str2=s2=(unsigned short*)str4;
    do {
        *s2=(unsigned short)*str4;
	s2++;
    } while (*str4++ != L'\0');

    return str2;
}

#ifndef HAVE_ECVT
/***********************************************************************
 *		ecvt
 */
char *ecvt (double number, int  ndigits,  int  *decpt,  int *sign)
{
    static char buf[40]; /* ought to be enough */
    char *dec;
    sprintf(buf, "%.*e", ndigits /* FIXME wrong */, number);
    *sign = (number < 0);
    dec = strchr(buf, '.');
    *decpt = (dec) ? (int)dec - (int)buf : -1;
    return buf;
}
#endif /* HAVE_ECVT */

#ifndef HAVE_FCVT
/***********************************************************************
 *		fcvt
 */
char *fcvt (double number, int  ndigits,  int  *decpt,  int *sign)
{
    static char buf[40]; /* ought to be enough */
    char *dec;
    sprintf(buf, "%.*e", ndigits, number);
    *sign = (number < 0);
    dec = strchr(buf, '.');
    *decpt = (dec) ? (int)dec - (int)buf : -1;
    return buf;
}
#endif /* HAVE_FCVT */

#ifndef HAVE_GCVT
/***********************************************************************
 *		gcvt
 *
 * FIXME: uses both E and F.
 */
char *gcvt (double number, size_t  ndigit,  char *buff)
{
    sprintf(buff, "%.*E", (int)ndigit, number);
    return buff;
}
#endif /* HAVE_GCVT */


#ifndef wine_memcpy_unaligned
/***********************************************************************
 *		wine_memcpy_unaligned
 *
 * This is necessary to defeat optimizations of memcpy by gcc.
 */
void *wine_memcpy_unaligned( void *dst, const void *src, size_t size )
{
    return memcpy( dst, src, size );
}
#endif


/***********************************************************************
 *		pthread functions
 */

#ifndef HAVE_PTHREAD_GETSPECIFIC
void pthread_getspecific() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_KEY_CREATE
void pthread_key_create() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_MUTEX_LOCK
void pthread_mutex_lock() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_MUTEX_UNLOCK
void pthread_mutex_unlock() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_SETSPECIFIC
void pthread_setspecific() { assert(0); }
#endif

/***********************************************************************
 *		interlocked functions
 */
#ifdef __i386__

#ifdef __GNUC__

__ASM_GLOBAL_FUNC(interlocked_cmpxchg,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_cmpxchg_ptr,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_xchg,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_xchg_ptr,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_xchg_add,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "ret");

#elif defined(_MSC_VER)

__declspec(naked) long interlocked_cmpxchg( long *dest, long xchg, long compare )
{
    __asm mov eax, 12[esp];
    __asm mov ecx, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock cmpxchg [edx], ecx;
    __asm ret;
}

__declspec(naked) void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare )
{
    __asm mov eax, 12[esp];
    __asm mov ecx, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock cmpxchg [edx], ecx;
    __asm ret;
}

__declspec(naked) long interlocked_xchg( long *dest, long val )
{
    __asm mov eax, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock xchg [edx], eax;
    __asm ret;
}

__declspec(naked) void *interlocked_xchg_ptr( void **dest, void *val )
{
    __asm mov eax, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock xchg [edx], eax;
    __asm ret;
}

__declspec(naked) long interlocked_xchg_add( long *dest, long incr )
{
    __asm mov eax, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock xadd [edx], eax;
    __asm ret;
}

#else
# error You must implement the interlocked* functions for your compiler
#endif

#elif defined(__powerpc__)
void* interlocked_cmpxchg_ptr( void **dest, void* xchg, void* compare)
{
    long ret = 0;
    long scratch;
    __asm__ __volatile__(
    "0:    lwarx %0,0,%2 ;"
    "      xor. %1,%4,%0;"
    "      bne 1f;"
    "      stwcx. %3,0,%2;"
    "      bne- 0b;"
    "1:    "
    : "=&r"(ret), "=&r"(scratch)
    : "r"(dest), "r"(xchg), "r"(compare)
    : "cr0","memory");
    return (void*)ret;
}

long interlocked_cmpxchg( long *dest, long xchg, long compare)
{
    long ret = 0;
    long scratch;
    __asm__ __volatile__(
    "0:    lwarx %0,0,%2 ;"
    "      xor. %1,%4,%0;"
    "      bne 1f;"
    "      stwcx. %3,0,%2;"
    "      bne- 0b;"
    "1:    "
    : "=&r"(ret), "=&r"(scratch)
    : "r"(dest), "r"(xchg), "r"(compare)
    : "cr0","memory");
    return ret;
}

long interlocked_xchg_add( long *dest, long incr )
{
    long ret = 0;
    long zero = 0;
    __asm__ __volatile__(
	"0:    lwarx %0, %3, %1;"
	"      add %0, %2, %0;"
	"      stwcx. %0, %3, %1;"
	"      bne- 0b;"
	: "=&r" (ret)
	: "r"(dest), "r"(incr), "r"(zero)
	: "cr0", "memory"
    );
    return ret-incr;
}

long interlocked_xchg( long* dest, long val )
{
    long ret = 0;
    __asm__ __volatile__(
    "0:    lwarx %0,0,%1 ;"
    "      stwcx. %2,0,%1;"
    "      bne- 0b;"
    : "=&r"(ret)
    : "r"(dest), "r"(val)
    : "cr0","memory");
    return ret;
}

void* interlocked_xchg_ptr( void** dest, void* val )
{
    void *ret = NULL;
    __asm__ __volatile__(
    "0:    lwarx %0,0,%1 ;"
    "      stwcx. %2,0,%1;"
    "      bne- 0b;"
    : "=&r"(ret)
    : "r"(dest), "r"(val)
    : "cr0","memory");
    return ret;
}

#elif defined(__sparc__) && defined(__sun__)

/*
 * As the earlier Sparc processors lack necessary atomic instructions,
 * I'm simply falling back to the library-provided _lwp_mutex routines
 * to ensure mutual exclusion in a way appropriate for the current
 * architecture.
 *
 * FIXME:  If we have the compare-and-swap instruction (Sparc v9 and above)
 *         we could use this to speed up the Interlocked operations ...
 */
#include <synch.h>
static lwp_mutex_t interlocked_mutex = DEFAULTMUTEX;

long interlocked_cmpxchg( long *dest, long xchg, long compare )
{
    _lwp_mutex_lock( &interlocked_mutex );
    if (*dest == compare) *dest = xchg;
    else compare = *dest;
    _lwp_mutex_unlock( &interlocked_mutex );
    return compare;
}

void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare )
{
    _lwp_mutex_lock( &interlocked_mutex );
    if (*dest == compare) *dest = xchg;
    else compare = *dest;
    _lwp_mutex_unlock( &interlocked_mutex );
    return compare;
}

long interlocked_xchg( long *dest, long val )
{
    long retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest = val;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}

void *interlocked_xchg_ptr( void **dest, void *val )
{
    long retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest = val;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}

long interlocked_xchg_add( long *dest, long incr )
{
    long retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest += incr;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}
#else
# error You must implement the interlocked* functions for your CPU
#endif
