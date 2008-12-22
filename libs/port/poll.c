/*
 * poll function
 *
 * Copyright 2008 Alexandre Julliard
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

#ifndef HAVE_POLL

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int poll( struct pollfd *fds, unsigned int count, int timeout )
{
    fd_set read_set, write_set, except_set;
    unsigned int i;
    int maxfd = -1, ret;

    FD_ZERO( &read_set );
    FD_ZERO( &write_set );
    FD_ZERO( &except_set );

    for (i = 0; i < count; i++)
    {
        if (fds[i].fd == -1) continue;
        if (fds[i].events & (POLLIN|POLLPRI)) FD_SET( fds[i].fd, &read_set  );
        if (fds[i].events & POLLOUT) FD_SET( fds[i].fd, &write_set );
        FD_SET( fds[i].fd, &except_set );  /* POLLERR etc. are always selected */
        if (fds[i].fd > maxfd) maxfd = fds[i].fd;
    }
    if (timeout != -1)
    {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout % 1000;
        ret = select( maxfd + 1, &read_set, &write_set, &except_set, &tv );
    }
    else ret = select( maxfd + 1, &read_set, &write_set, &except_set, NULL );

    if (ret >= 0)
    {
        for (i = 0; i < count; i++)
        {
            fds[i].revents = 0;
            if (fds[i].fd == -1) continue;
            if (FD_ISSET( fds[i].fd, &read_set )) fds[i].revents |= POLLIN;
            if (FD_ISSET( fds[i].fd, &write_set )) fds[i].revents |= POLLOUT;
            if (FD_ISSET( fds[i].fd, &except_set )) fds[i].revents |= POLLERR;
        }
    }
    return ret;
}

#endif /* HAVE_POLL */
