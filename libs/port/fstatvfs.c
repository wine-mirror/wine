/*
 * fstatvfs function
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

#ifndef HAVE_FSTATVFS

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif

int fstatvfs( int fd, struct statvfs *buf )
{
    int ret;
#ifdef HAVE_FSTATFS
    struct statfs info;

/* FIXME: add autoconf check for this */
#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)
    ret = fstatfs( fd, &info, 0, 0 );
#else
    ret = fstatfs( fd, &info );
#endif
    if (ret >= 0)
    {
        memset( buf, 0, sizeof(*buf) );
        buf->f_bsize   = info.f_bsize;
        buf->f_blocks  = info.f_blocks;
        buf->f_files   = info.f_files;
#ifdef HAVE_STRUCT_STATFS_F_NAMELEN
        buf->f_namemax = info.f_namelen;
#endif
#ifdef HAVE_STRUCT_STATFS_F_FRSIZE
        buf->f_frsize  = info.f_frsize;
#else
        buf->f_frsize  = info.f_bsize;
#endif
#ifdef HAVE_STRUCT_STATFS_F_BFREE
        buf->f_bfree   = info.f_bfree;
#else
        buf->f_bfree   = info.f_bavail;
#endif
#ifdef HAVE_STRUCT_STATFS_F_BAVAIL
        buf->f_bavail  = info.f_bavail;
#else
        buf->f_bavail  = info.f_bfree;
#endif
#ifdef HAVE_STRUCT_STATFS_F_FFREE
        buf->f_ffree   = info.f_ffree;
#else
        buf->f_ffree   = info.f_favail;
#endif
#ifdef HAVE_STRUCT_STATFS_F_FAVAIL
        buf->f_favail  = info.f_favail;
#else
        buf->f_favail  = info.f_ffree;
#endif
    }
#else  /* HAVE_FSTATFS */
    ret = -1;
    errno = ENOSYS;
#endif  /* HAVE_FSTATFS */
    return ret;
}

#endif /* HAVE_FSTATVFS */
