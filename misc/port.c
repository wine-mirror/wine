/*
 * Misc. functions for systems that don't have them
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "wine/port.h"

#ifdef __BEOS__
#include <be/kernel/fs_info.h>
#include <be/kernel/OS.h>
#endif

#include <assert.h>
#include <ctype.h>
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
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#endif
#ifdef HAVE_PTY_H
# include <pty.h>
#endif

/***********************************************************************
 *		usleep
 */
#ifndef HAVE_USLEEP
unsigned int usleep (unsigned int useconds)
{
#if defined(__EMX__)
    DosSleep(useconds);
    return 0;
#elif defined(__BEOS__)
    return snooze(useconds);
#elif defined(HAVE_SELECT)
    struct timeval delay;

    delay.tv_sec = 0;
    delay.tv_usec = useconds;

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
    while (*str1 && toupper(*str1) == toupper(*str2)) { str1++; str2++; }
    return toupper(*str1) - toupper(*str2);
}
#endif /* HAVE_STRCASECMP */

/***********************************************************************
 *		strncasecmp
 */
#ifndef HAVE_STRNCASECMP
int strncasecmp( const char *str1, const char *str2, size_t n )
{
    int res;
    if (!n) return 0;
    while ((--n > 0) && *str1)
      if ((res = toupper(*str1++) - toupper(*str2++))) return res;
    return toupper(*str1) - toupper(*str2);
}
#endif /* HAVE_STRNCASECMP */

/***********************************************************************
 *		wine_openpty
 * NOTE
 *   It looks like the openpty that comes with glibc in RedHat 5.0
 *   is buggy (second call returns what looks like a dup of 0 and 1
 *   instead of a new pty), this is a generic replacement.
 *
 * FIXME
 *   We should have a autoconf check for this.
 */
int wine_openpty(int *master, int *slave, char *name, 
			struct termios *term, struct winsize *winsize)
{
#ifdef HAVE_OPENPTY
    return openpty(master,slave,name,term,winsize);
#else
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
#endif
}

/***********************************************************************
 *		getnetbyaddr
 */
#ifndef HAVE_GETNETBYADDR
struct netent *getnetbyaddr(unsigned long net, int type)
{
    errno = ENOSYS;
    return NULL;
}
#endif /* defined(HAVE_GETNETBYNAME) */

/***********************************************************************
 *		getnetbyname
 */
#ifndef HAVE_GETNETBYNAME
struct netent *getnetbyname(const char *name)
{
    errno = ENOSYS;
    return NULL;
}
#endif /* defined(HAVE_GETNETBYNAME) */

/***********************************************************************
 *		getprotobyname
 */
#ifndef HAVE_GETPROTOBYNAME
struct protoent *getprotobyname(const char *name)
{
    errno = ENOSYS;
    return NULL;
}
#endif /* !defined(HAVE_GETPROTOBYNAME) */

/***********************************************************************
 *		getprotobynumber
 */
#ifndef HAVE_GETPROTOBYNUMBER
struct protoent *getprotobynumber(int proto)
{
    errno = ENOSYS;
    return NULL;
}
#endif /* !defined(HAVE_GETPROTOBYNUMBER) */

/***********************************************************************
 *		getservbyport
 */
#ifndef HAVE_GETSERVBYPORT
struct servent *getservbyport(int port, const char *proto)
{
    errno = ENOSYS;
    return NULL;
}
#endif /* !defined(HAVE_GETSERVBYPORT) */

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
 *		settimeofday
 */
#ifndef HAVE_SETTIMEOFDAY
int settimeofday(struct timeval *tp, void *reserved)
{
    tp->tv_sec = 0;
    tp->tv_usec = 0;

    errno = ENOSYS;
    return -1;
}
#endif /* HAVE_SETTIMEOFDAY */

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
