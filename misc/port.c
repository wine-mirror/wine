/*
 * Misc. functions for systems that don't have them
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"
#include <sys/types.h>
#include <sys/time.h>

#ifndef HAVE_USLEEP
unsigned int usleep (unsigned int useconds)
{
    struct timeval delay;

    delay.tv_sec = 0;
    delay.tv_usec = useconds;

    select( 0, 0, 0, 0, &delay );
    return 0;
}
#endif /* HAVE_USLEEP */

#ifndef HAVE_MEMMOVE
void *memmove( void *dst, const void *src, unsigned int len )
{
    /* Use memcpy if not overlapping */
    if (((char *)dst + len <= (char *)src) ||
        ((char *)src + len <= (char *)dst))
    {
        memcpy( dst, src, len );
    }
    /* Otherwise do it the hard way (FIXME: could do better than this) */
    else if (dst < src)
    {
        while (len--) *((char *)dst)++ = *((char *)src)++;
    }
    else
    {
        dst = (char *)dst + len - 1;
        src = (char *)src + len - 1;
        while (len--) *((char *)dst)-- = *((char *)src)--;
    }
    return dst;
}
#endif  /* HAVE_MEMMOVE */
