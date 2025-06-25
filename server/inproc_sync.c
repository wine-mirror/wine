/*
 * In-process synchronization primitives
 *
 * Copyright (C) 2021-2022 Elizabeth Figura for CodeWeavers
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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "request.h"
#include "thread.h"

#ifdef HAVE_LINUX_NTSYNC_H
# include <linux/ntsync.h>
#endif

#ifdef NTSYNC_IOC_EVENT_READ

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

int get_inproc_device_fd(void)
{
    static int fd = -2;
    if (fd == -2) fd = open( "/dev/ntsync", O_CLOEXEC | O_RDONLY );
    return fd;
}

struct inproc_sync
{
    struct object          obj;  /* object header */
    enum inproc_sync_type  type;
    int                    fd;
};

static void inproc_sync_dump( struct object *obj, int verbose );
static void inproc_sync_destroy( struct object *obj );

static const struct object_ops inproc_sync_ops =
{
    sizeof(struct inproc_sync), /* size */
    &no_type,                   /* type */
    inproc_sync_dump,           /* dump */
    no_add_queue,               /* add_queue */
    NULL,                       /* remove_queue */
    NULL,                       /* signaled */
    NULL,                       /* satisfied */
    no_signal,                  /* signal */
    no_get_fd,                  /* get_fd */
    default_get_sync,           /* get_sync */
    default_map_access,         /* map_access */
    default_get_sd,             /* get_sd */
    default_set_sd,             /* set_sd */
    default_get_full_name,      /* get_full_name */
    no_lookup_name,             /* lookup_name */
    directory_link_name,        /* link_name */
    default_unlink_name,        /* unlink_name */
    no_open_file,               /* open_file */
    no_kernel_obj_list,         /* get_kernel_obj_list */
    no_close_handle,            /* close_handle */
    inproc_sync_destroy,        /* destroy */
};

struct inproc_sync *create_inproc_event_sync( int manual, int signaled )
{
    struct ntsync_event_args args = {.signaled = signaled, .manual = manual};
    struct inproc_sync *event;

    if (!(event = alloc_object( &inproc_sync_ops ))) return NULL;
    event->type = INPROC_SYNC_EVENT;
    event->fd = ioctl( get_inproc_device_fd(), NTSYNC_IOC_CREATE_EVENT, &args );

    return event;
}

static void inproc_sync_dump( struct object *obj, int verbose )
{
    struct inproc_sync *sync = (struct inproc_sync *)obj;
    assert( obj->ops == &inproc_sync_ops );
    fprintf( stderr, "Inproc sync type=%d, fd=%d\n", sync->type, sync->fd );
}

void signal_inproc_sync( struct inproc_sync *sync )
{
    __u32 count;
    if (debug_level) fprintf( stderr, "set_inproc_event %d\n", sync->fd );
    ioctl( sync->fd, NTSYNC_IOC_EVENT_SET, &count );
}

void reset_inproc_sync( struct inproc_sync *sync )
{
    __u32 count;
    if (debug_level) fprintf( stderr, "reset_inproc_event %d\n", sync->fd );
    ioctl( sync->fd, NTSYNC_IOC_EVENT_RESET, &count );
}

static void inproc_sync_destroy( struct object *obj )
{
    struct inproc_sync *sync = (struct inproc_sync *)obj;
    assert( obj->ops == &inproc_sync_ops );
    close( sync->fd );
}

#else /* NTSYNC_IOC_EVENT_READ */

int get_inproc_device_fd(void)
{
    return -1;
}

struct inproc_sync *create_inproc_event_sync( int manual, int signaled )
{
    return NULL;
}

void signal_inproc_sync( struct inproc_sync *sync )
{
}

void reset_inproc_sync( struct inproc_sync *sync )
{
}

#endif /* NTSYNC_IOC_EVENT_READ */
