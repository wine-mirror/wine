/*
 * mkstemp function
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

#include <fcntl.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef HAVE_MKSTEMP
int mkstemp(char *tmpfn)
{
    int tries;
    char *xstart;

    xstart = tmpfn+strlen(tmpfn)-1;
    while ((xstart > tmpfn) && (*xstart == 'X'))
        xstart--;
    tries = 10;
    while (tries--)
    {
        char *newfn = mktemp(tmpfn);
        int fd;
        if (!newfn) /* something else broke horribly */
            return -1;
        fd = open(newfn,O_CREAT|O_RDWR|O_EXCL,0600);
        if (fd!=-1)
            return fd;
        newfn = xstart;
        /* fill up with X and try again ... */
        while (*newfn) *newfn++ = 'X';
    }
    return -1;
}
#endif /* HAVE_MKSTEMP */
