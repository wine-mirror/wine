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
