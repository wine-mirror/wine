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
#include "wine/unixlib.h"

enum device_op
{
    ADD_DOS_DEVICE,
    ADD_VOLUME,
    REMOVE_DEVICE
};

struct device_info
{
    enum device_op    op;
    enum device_type  type;
    const char       *udi;
    const char       *device;
    const char       *mount_point;
    const char       *serial;
    const char       *label;
    GUID             *guid;
    struct scsi_info *scsi_info;

    /* buffer space for pointers */
    GUID              guid_buffer;
    struct scsi_info  scsi_buffer;
    char              str_buffer[1024];
};

struct run_loop_params
{
    HANDLE     op_thread;
    PNTAPCFUNC op_apc;
};

struct dequeue_device_op_params
{
    ULONG_PTR arg;
    struct device_info *info;
};

struct add_drive_params
{
    const char *device;
    enum device_type type;
    int *letter;
};

struct size_info
{
    LONGLONG total_allocation_units;
    LONGLONG caller_available_allocation_units;
    LONGLONG actual_available_allocation_units;
    ULONG sectors_per_allocation_unit;
    ULONG bytes_per_sector;
};

struct get_volume_size_info_params
{
    const char *unix_mount;
    struct size_info *info;
};

struct get_dosdev_symlink_params
{
    const char *dev;
    char *dest;
    ULONG size;
};

struct set_dosdev_symlink_params
{
    const char *dev;
    const char *dest;
};

struct get_volume_dos_devices_params
{
    const char *mount_point;
    unsigned int *dosdev;
};

struct read_volume_file_params
{
    const char *volume;
    const char *file;
    void *buffer;
    ULONG *size;
};

struct match_unixdev_params
{
    const char *device;
    ULONGLONG unix_dev;
};

struct detect_ports_params
{
    char *names;
    ULONG size;
};

struct set_shell_folder_params
{
    const WCHAR *folder;
    const char *link;
    BOOL create_backup;
};

struct get_shell_folder_params
{
    const char *folder;
    char *buffer;
    ULONG size;
};

struct dhcp_request_params
{
    const char *unix_name;
    struct mountmgr_dhcp_request_param *req;
    char  *buffer;
    ULONG  offset;
    ULONG  size;
    ULONG *ret_size;
};

struct ioctl_params
{
    void  *buff;
    ULONG  insize;
    ULONG  outsize;
    ULONG *info;
};

enum mountmgr_funcs
{
    unix_run_loop,
    unix_dequeue_device_op,
    unix_add_drive,
    unix_get_dosdev_symlink,
    unix_set_dosdev_symlink,
    unix_get_volume_size_info,
    unix_get_volume_dos_devices,
    unix_read_volume_file,
    unix_match_unixdev,
    unix_check_device_access,
    unix_detect_serial_ports,
    unix_detect_parallel_ports,
    unix_set_shell_folder,
    unix_get_shell_folder,
    unix_dhcp_request,
    unix_query_symbol_file,
    unix_read_credential,
    unix_write_credential,
    unix_delete_credential,
    unix_enumerate_credentials,
    unix_funcs_count
};

#define MOUNTMGR_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

extern void queue_device_op( enum device_op op, const char *udi, const char *device,
                             const char *mount_point, enum device_type type, const GUID *guid,
                             const char *disk_serial, const char *label,
                             const struct scsi_info *info );
extern void run_dbus_loop(void);
extern void run_diskarbitration_loop(void);

extern NTSTATUS dhcp_request( void *args );
extern NTSTATUS query_symbol_file( void *args );
extern NTSTATUS read_credential( void *args );
extern NTSTATUS write_credential( void *args );
extern NTSTATUS delete_credential( void *args );
extern NTSTATUS enumerate_credentials( void *args );
