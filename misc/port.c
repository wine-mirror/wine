/*
 * Misc. functions for systems that don't have them
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"
#include <sys/types.h>
#include <sys/time.h>

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
