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

#ifndef __WINE_MOUNTMGR_H
#define __WINE_MOUNTMGR_H

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddscsi.h"
#include "ntddcdrm.h"
#include "ddk/wdm.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"

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

extern NTSTATUS WINAPI harddisk_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path );
extern NTSTATUS WINAPI serial_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path );
extern NTSTATUS WINAPI parallel_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path );

enum scsi_device_type
{
    SCSI_DISK_PERIPHERAL = 0x00,
    SCSI_TAPE_PERIPHERAL = 0x01,
    SCSI_PRINTER_PERIPHERAL = 0x02,
    SCSI_PROCESSOR_PERIPHERAL = 0x03,
    SCSI_WORM_PERIPHERAL = 0x04,
    SCSI_CDROM_PERIPHERAL = 0x05,
    SCSI_SCANNER_PERIPHERAL = 0x06,
    SCSI_OPTICAL_DISK_PERIPHERAL = 0x07,
    SCSI_MEDIUM_CHANGER_PERIPHERAL = 0x08,
    SCSI_COMMS_PERIPHERAL = 0x09,
    SCSI_ASC_GRAPHICS_PERIPHERAL = 0x0A,
    SCSI_ASC_GRAPHICS2_PERIPHERAL = 0x0B,
    SCSI_ARRAY_PERIPHERAL = 0x0C,
    SCSI_ENCLOSURE_PERIPHERAL = 0x0D,
    SCSI_REDUCED_DISK_PERIPHERAL = 0x0E,
    SCSI_CARD_READER_PERIPHERAL = 0x0F,
    SCSI_BRIDGE_PERIPHERAL = 0x10,
    SCSI_OBJECT_STORAGE_PERIPHERAL = 0x11,
    SCSI_DRIVE_CONTROLLER_PERIPHERAL = 0x12,
    SCSI_REDUCED_CDROM_PERIPHERAL = 0x15,
    SCSI_UNKNOWN_PERIPHERAL = ~0
};

struct scsi_info
{
    enum scsi_device_type type;
    SCSI_ADDRESS          addr;
    UINT                  init_id;
    char                  driver[64];
    char                  model[64];
};

extern NTSTATUS add_volume( const char *udi, const char *device, const char *mount_point,
                            enum device_type type, const GUID *guid, const char *disk_serial,
                            const struct scsi_info *info );
extern NTSTATUS remove_volume( const char *udi );
extern NTSTATUS add_dos_device( int letter, const char *udi, const char *device,
                                const char *mount_point, enum device_type type, const GUID *guid,
                                const struct scsi_info *info );
extern NTSTATUS remove_dos_device( int letter, const char *udi );
extern NTSTATUS query_unix_drive( void *buff, SIZE_T insize, SIZE_T outsize,
                                  IO_STATUS_BLOCK *iosb );

/* mount point functions */

struct mount_point;

extern struct mount_point *add_dosdev_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                                   int drive );
extern struct mount_point *add_volume_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                                   const GUID *guid );
extern void delete_mount_point( struct mount_point *mount );
extern void set_mount_point_id( struct mount_point *mount, const void *id, unsigned int id_len );

#endif /* __WINE_MOUNTMGR_H */
