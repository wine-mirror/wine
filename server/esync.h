/*
 * eventfd-based synchronization objects
 *
 * Copyright (C) 2018 Zebediah Figura
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

#include <unistd.h>

extern int do_esync(void);
void esync_init(void);

/* This is an eventfd or something that behaves like one. */
struct esync_fd;

struct esync_fd *esync_create_fd( int initval, int semaphore );
void esync_close_fd( struct esync_fd *fd );
void esync_wake_fd( struct esync_fd *fd );
void esync_wake_up( struct object *obj );
void esync_clear( struct esync_fd *fd );

struct esync;

extern const struct object_ops esync_ops;
void esync_set_event( struct esync *esync );
void esync_reset_event( struct esync *esync );
void esync_abandon_mutexes( struct thread *thread );
