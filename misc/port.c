/*
 * Misc. functions for systems that don't have them
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#ifdef HAVE_LIBIO_H
# include <libio.h>
#endif

#ifndef HAVE_USLEEP
#ifdef __EMX__
unsigned int usleep (unsigned int useconds) { DosSleep(useconds); }
#else
unsigned int usleep (unsigned int useconds)
{
    struct timeval delay;

    delay.tv_sec = 0;
    delay.tv_usec = useconds;

    select( 0, 0, 0, 0, &delay );
    return 0;
}
#endif
#endif /* HAVE_USLEEP */

#ifndef HAVE_MEMMOVE
void *memmove( void *dest, const void *src, unsigned int len )
{
    register char *dst = dest;

    /* Use memcpy if not overlapping */
    if ((dst + len <= (char *)src) || ((char *)src + len <= dst))
    {
        memcpy( dst, src, len );
    }
    /* Otherwise do it the hard way (FIXME: could do better than this) */
    else if (dst < src)
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

#ifndef HAVE_STRERROR
const char *strerror( int err )
{
    /* Let's hope we have sys_errlist then */
    return sys_errlist[err];
}
#endif  /* HAVE_STRERROR */

#if !defined(HAVE_CLONE) && defined(__linux__)
#include <assert.h>
#include <errno.h>
#include <syscall.h>
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


/** 
 *  It looks like the openpty that comes with glibc in RedHat 5.0
 *  is buggy (second call returns what looks like a dup of 0 and 1
 *  instead of a new pty), this is a generic replacement.
 */
/** We will have an autoconf check for this soon... */

int wine_openpty(int *master, int *slave, char *name, 
			struct termios *term, struct winsize *winsize)
{
    char *ptr1, *ptr2;
    char pts_name[512];

    strcpy (pts_name, "/dev/ptyXY");

    for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1 != 0; ptr1++) {
        pts_name[8] = *ptr1;
        for (ptr2 = "0123456789abcdef"; *ptr2 != 0; ptr2++) {
            pts_name[9] = *ptr2;

            if ((*master = open(pts_name, O_RDWR)) < 0) {
                if (errno == ENOENT)
                    return -1;
                else
                    continue;
            }
            pts_name[5] = 't';
            if ((*slave = open(pts_name, O_RDWR)) < 0) {
                pts_name[5] = 'p';
                continue;
            }

            if (term != NULL)
	        tcsetattr(*slave, TCSANOW, term);
	    if (winsize != NULL)
	        ioctl(*slave, TIOCSWINSZ, winsize);
	    if (name != NULL)
	        strcpy(name, pts_name);
            return *slave;
        }
    }
    return -1;
}

