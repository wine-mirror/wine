/*
 * statfs function
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

#ifdef __BEOS__
#include <be/kernel/fs_info.h>
#include <be/kernel/OS.h>
#endif

#include <errno.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef STATFS_DEFINED_BY_SYS_VFS
# include <sys/vfs.h>
#else
# ifdef STATFS_DEFINED_BY_SYS_MOUNT
#  include <sys/mount.h>
# else
#  ifdef STATFS_DEFINED_BY_SYS_STATFS
#   include <sys/statfs.h>
#  endif
# endif
#endif

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
