/*
 * usleep function
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

#include <errno.h>

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
