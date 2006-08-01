/*
 * memmove function
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

#include "config.h"
#include "wine/port.h"

#ifndef HAVE_MEMMOVE
void *memmove( void *dest, const void *source, size_t len )
{
    register char *dst = dest;
    register const char *src = source;

    /* Use memcpy if not overlapping */
    if ((dst + len <= src) || (src + len <= dst))
    {
        memcpy( dst, src, len );
    }
    /* Otherwise do it the hard way (FIXME: could do better than this) */
    else if (dst < src)
    {
        while (len--) *dst++ = *src++;
    }
    else
    {
        dst += len - 1;
        src += len - 1;
        while (len--) *dst-- = *src--;
    }
    return dest;
}
#endif  /* HAVE_MEMMOVE */
