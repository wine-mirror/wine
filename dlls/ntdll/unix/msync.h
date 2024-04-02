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

extern int do_msync(void) DECLSPEC_HIDDEN;
extern void msync_init(void) DECLSPEC_HIDDEN;
extern NTSTATUS msync_close( HANDLE handle ) DECLSPEC_HIDDEN;

extern NTSTATUS msync_create_semaphore(HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, LONG initial, LONG max) DECLSPEC_HIDDEN;
extern NTSTATUS msync_release_semaphore( HANDLE handle, ULONG count, ULONG *prev ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_open_semaphore( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_query_semaphore( HANDLE handle, void *info, ULONG *ret_len ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_create_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, EVENT_TYPE type, BOOLEAN initial ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_open_event( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_set_event( HANDLE handle, LONG *prev ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_reset_event( HANDLE handle, LONG *prev ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_pulse_event( HANDLE handle, LONG *prev ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_query_event( HANDLE handle, void *info, ULONG *ret_len ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_create_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr, BOOLEAN initial ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_open_mutex( HANDLE *handle, ACCESS_MASK access,
    const OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_release_mutex( HANDLE handle, LONG *prev ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_query_mutex( HANDLE handle, void *info, ULONG *ret_len ) DECLSPEC_HIDDEN;

extern NTSTATUS msync_wait_objects( DWORD count, const HANDLE *handles, BOOLEAN wait_any,
                                    BOOLEAN alertable, const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;
extern NTSTATUS msync_signal_and_wait( HANDLE signal, HANDLE wait,
    BOOLEAN alertable, const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;
