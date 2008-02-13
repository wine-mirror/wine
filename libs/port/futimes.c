/*
 * futimes function
 *
 * Copyright 2004 Alexandre Julliard
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

#ifndef HAVE_FUTIMES

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#include <stdio.h>
#include <errno.h>

int futimes(int fd, const struct timeval tv[2])
{
#ifdef linux
    char buffer[sizeof("/proc/self/fd/") + 3*sizeof(int)];

    sprintf( buffer, "/proc/self/fd/%u", fd );
    if (tv)
    {
        struct utimbuf ut;
        ut.actime  = tv[0].tv_sec + (tv[0].tv_usec + 500000) / 1000000;
        ut.modtime = tv[1].tv_sec + (tv[1].tv_usec + 500000) / 1000000;
        return utime( buffer, &ut );
    }
    else return utime( buffer, NULL );
#elif defined(HAVE_FUTIMESAT)
    return futimesat( fd, NULL, tv );
#else
    errno = ENOSYS;
    return -1;
#endif
}

#endif  /* HAVE_FUTIMES */
