/*
 * pwrite function
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
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_IO_H
# include <io.h>
#endif

/* FIXME: this is not thread-safe */

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
