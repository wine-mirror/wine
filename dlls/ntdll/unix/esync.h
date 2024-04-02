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

extern int do_esync(void) DECLSPEC_HIDDEN;
extern void esync_init(void) DECLSPEC_HIDDEN;
extern NTSTATUS esync_close( HANDLE handle ) DECLSPEC_HIDDEN;

extern NTSTATUS esync_create_semaphore(HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, LONG initial, LONG max) DECLSPEC_HIDDEN;
extern NTSTATUS esync_open_semaphore( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_query_semaphore( HANDLE handle, void *info, ULONG *ret_len ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_release_semaphore( HANDLE handle, ULONG count, ULONG *prev ) DECLSPEC_HIDDEN;

extern NTSTATUS esync_create_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, EVENT_TYPE type, BOOLEAN initial ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_open_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_pulse_event( HANDLE handle ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_query_event( HANDLE handle, void *info, ULONG *ret_len ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_reset_event( HANDLE handle ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_set_event( HANDLE handle ) DECLSPEC_HIDDEN;

extern NTSTATUS esync_create_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, BOOLEAN initial ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_open_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_query_mutex( HANDLE handle, void *info, ULONG *ret_len ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_release_mutex( HANDLE *handle, LONG *prev ) DECLSPEC_HIDDEN;

extern NTSTATUS esync_wait_objects( DWORD count, const HANDLE *handles, BOOLEAN wait_any,
                                    BOOLEAN alertable, const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;
extern NTSTATUS esync_signal_and_wait( HANDLE signal, HANDLE wait, BOOLEAN alertable,
    const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;


/* We have to synchronize on the fd cache mutex so that our calls to receive_fd
 * don't race with theirs. It looks weird, I know.
 *
 * If we weren't trying to avoid touching the code I'd rename the mutex to
 * "server_fd_mutex" or something similar. */
extern pthread_mutex_t fd_cache_mutex;

extern int receive_fd( obj_handle_t *handle ) DECLSPEC_HIDDEN;
