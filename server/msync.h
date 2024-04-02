/*
 * mach semaphore-based synchronization objects
 *
 * Copyright (C) 2018 Zebediah Figura
 * Copyright (C) 2023 Marc-Aurel Zent
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

extern int do_msync(void);
extern void msync_init(void);
extern unsigned int msync_alloc_shm( int low, int high );
extern void msync_signal_all( unsigned int shm_idx );
extern void msync_clear_shm( unsigned int shm_idx );
extern void msync_destroy_semaphore( unsigned int shm_idx );
extern void msync_wake_up( struct object *obj );
extern void msync_clear( struct object *obj );

struct msync;

extern const struct object_ops msync_ops;
extern void msync_set_event( struct msync *msync );
extern void msync_reset_event( struct msync *msync );
extern void msync_abandon_mutexes( struct thread *thread );
