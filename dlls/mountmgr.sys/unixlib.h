/*
 * MountMgr Unix interface definitions
 *
 * Copyright 2021 Alexandre Julliard
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

#include "mountmgr.h"

extern NTSTATUS add_drive( const char *device, enum device_type type, int *letter ) DECLSPEC_HIDDEN;
extern NTSTATUS get_dosdev_symlink( const char *dev, char *dest, ULONG size ) DECLSPEC_HIDDEN;
extern NTSTATUS set_dosdev_symlink( const char *dev, const char *dest ) DECLSPEC_HIDDEN;
extern NTSTATUS get_volume_dos_devices( const char *mount_point, unsigned int *dosdev ) DECLSPEC_HIDDEN;
extern NTSTATUS read_volume_file( const char *volume, const char *file, void *buffer, ULONG *size ) DECLSPEC_HIDDEN;
extern BOOL match_unixdev( const char *device, ULONGLONG unix_dev ) DECLSPEC_HIDDEN;
extern NTSTATUS detect_serial_ports( char *names, ULONG size ) DECLSPEC_HIDDEN;
extern NTSTATUS detect_parallel_ports( char *names, ULONG size ) DECLSPEC_HIDDEN;
extern NTSTATUS set_shell_folder( const char *folder, const char *backup, const char *link ) DECLSPEC_HIDDEN;
extern NTSTATUS get_shell_folder( const char *folder, char *buffer, ULONG size ) DECLSPEC_HIDDEN;
