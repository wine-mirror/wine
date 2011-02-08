/*
 * Mountmanager private header
 *
 * Copyright 2008 Maarten Lankhorst
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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
#include "ddk/wdm.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"

extern void initialize_hal(void);
extern void initialize_diskarbitration(void);

/* device functions */

enum device_type
{
    DEVICE_UNKNOWN,
    DEVICE_HARDDISK,
    DEVICE_HARDDISK_VOL,
    DEVICE_FLOPPY,
    DEVICE_CDROM,
    DEVICE_DVD,
    DEVICE_NETWORK,
    DEVICE_RAMDISK
};

extern NTSTATUS add_volume( const char *udi, const char *device, const char *mount_point,
                            enum device_type type, const GUID *guid );
extern NTSTATUS remove_volume( const char *udi );
extern NTSTATUS add_dos_device( int letter, const char *udi, const char *device,
                                const char *mount_point, enum device_type type, const GUID *guid );
extern NTSTATUS remove_dos_device( int letter, const char *udi );
extern NTSTATUS query_dos_device( int letter, enum device_type *type, char **device, char **mount_point );
extern NTSTATUS WINAPI harddisk_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path );

/* mount point functions */

struct mount_point;

extern struct mount_point *add_dosdev_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                                   int drive );
extern struct mount_point *add_volume_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                                   const GUID *guid );
extern void delete_mount_point( struct mount_point *mount );
extern void set_mount_point_id( struct mount_point *mount, const void *id, unsigned int id_len );
