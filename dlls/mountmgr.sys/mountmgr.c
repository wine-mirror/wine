/*
 * Mount manager service implementation
 *
 * Copyright 2008 Alexandre Julliard
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
#include <unistd.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "winreg.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
#include "ddk/wdm.h"
#include "ddk/mountmgr.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "mountmgr.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#define MIN_ID_LEN     4
#define MAX_DOS_DRIVES 26

/* extra info for disk devices, stored in DeviceExtension */
struct disk_device_info
{
    UNICODE_STRING        name;     /* device name */
    STORAGE_DEVICE_NUMBER devnum;   /* device number info */
};

struct mount_point
{
    struct list    entry;   /* entry in mount points list */
    DEVICE_OBJECT *device;  /* disk device */
    UNICODE_STRING link;    /* DOS device symlink */
    void          *id;      /* device unique id */
    unsigned int   id_len;
};

static struct list mount_points_list = LIST_INIT(mount_points_list);
static HKEY mount_key;

static inline UNICODE_STRING *get_device_name( DEVICE_OBJECT *dev )
{
    return &((struct disk_device_info *)dev->DeviceExtension)->name;
}

/* read a Unix symlink; returned buffer must be freed by caller */
static char *read_symlink( const char *path )
{
    char *buffer;
    int ret, size = 128;

    for (;;)
    {
        if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
        ret = readlink( path, buffer, size );
        if (ret == -1)
        {
            RtlFreeHeap( GetProcessHeap(), 0, buffer );
            return 0;
        }
        if (ret != size)
        {
            buffer[ret] = 0;
            return buffer;
        }
        RtlFreeHeap( GetProcessHeap(), 0, buffer );
        size *= 2;
    }
}

static NTSTATUS create_disk_device( DRIVER_OBJECT *driver, DWORD type, DEVICE_OBJECT **dev_obj )
{
    static const WCHAR harddiskW[] = {'\\','D','e','v','i','c','e',
                                      '\\','H','a','r','d','d','i','s','k','V','o','l','u','m','e','%','u',0};
    static const WCHAR cdromW[] = {'\\','D','e','v','i','c','e','\\','C','d','R','o','m','%','u',0};
    static const WCHAR floppyW[] = {'\\','D','e','v','i','c','e','\\','F','l','o','p','p','y','%','u',0};

    UINT i, first = 0;
    NTSTATUS status = 0;
    const WCHAR *format;
    UNICODE_STRING name;
    struct disk_device_info *info;

    switch(type)
    {
    case DRIVE_REMOVABLE:
        format = floppyW;
        break;
    case DRIVE_CDROM:
        format = cdromW;
        break;
    case DRIVE_FIXED:
    default:  /* FIXME */
        format = harddiskW;
        first = 1;  /* harddisk volumes start counting from 1 */
        break;
    }

    name.MaximumLength = (strlenW(format) + 10) * sizeof(WCHAR);
    name.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, name.MaximumLength );
    for (i = first; i < 32; i++)
    {
        sprintfW( name.Buffer, format, i );
        name.Length = strlenW(name.Buffer) * sizeof(WCHAR);
        status = IoCreateDevice( driver, sizeof(*info), &name, 0, 0, FALSE, dev_obj );
        if (status != STATUS_OBJECT_NAME_COLLISION) break;
    }
    if (!status)
    {
        info = (*dev_obj)->DeviceExtension;
        info->name = name;
        switch(type)
        {
        case DRIVE_REMOVABLE:
            info->devnum.DeviceType = FILE_DEVICE_DISK;
            info->devnum.DeviceNumber = i;
            info->devnum.PartitionNumber = ~0u;
            break;
        case DRIVE_CDROM:
            info->devnum.DeviceType = FILE_DEVICE_CD_ROM;
            info->devnum.DeviceNumber = i;
            info->devnum.PartitionNumber = ~0u;
            break;
        case DRIVE_FIXED:
        default:  /* FIXME */
            info->devnum.DeviceType = FILE_DEVICE_DISK;
            info->devnum.DeviceNumber = 0;
            info->devnum.PartitionNumber = i;
            break;
        }
    }
    else
    {
        FIXME( "IoCreateDevice %s got %x\n", debugstr_w(name.Buffer), status );
        RtlFreeUnicodeString( &name );
    }
    return status;
}


static void set_mount_point_id( struct mount_point *mount, const void *id, unsigned int id_len )
{
    RtlFreeHeap( GetProcessHeap(), 0, mount->id );
    mount->id_len = max( MIN_ID_LEN, id_len );
    if ((mount->id = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, mount->id_len )))
    {
        memcpy( mount->id, id, id_len );
        RegSetValueExW( mount_key, mount->link.Buffer, 0, REG_BINARY, mount->id, mount->id_len );
    }
    else mount->id_len = 0;
}

static struct mount_point *add_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                            const WCHAR *link, const void *id, unsigned int id_len )
{
    struct mount_point *mount;
    WCHAR *str;
    UINT len = (strlenW(link) + 1) * sizeof(WCHAR);

    if (!(mount = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*mount) + len ))) return NULL;

    str = (WCHAR *)(mount + 1);
    strcpyW( str, link );
    RtlInitUnicodeString( &mount->link, str );
    mount->device = device;
    mount->id = NULL;
    list_add_tail( &mount_points_list, &mount->entry );

    IoCreateSymbolicLink( &mount->link, device_name );
    set_mount_point_id( mount, id, id_len );

    TRACE( "created %s id %s for %s\n", debugstr_w(mount->link.Buffer),
           debugstr_a(mount->id), debugstr_w(device_name->Buffer) );
    return mount;
}

/* create the DosDevices mount point symlink for a new device */
static struct mount_point *add_dosdev_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                                   int drive, const void *id, unsigned int id_len )
{
    static const WCHAR driveW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','%','c',':',0};
    WCHAR link[sizeof(driveW)];

    sprintfW( link, driveW, 'A' + drive );
    return add_mount_point( device, device_name, link, id, id_len );
}

/* create the Volume mount point symlink for a new device */
static struct mount_point *add_volume_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                                   int drive, const void *id, unsigned int id_len )
{
    static const WCHAR volumeW[] = {'\\','?','?','\\','V','o','l','u','m','e','{',
                                    '%','0','8','x','-','%','0','4','x','-','%','0','4','x','-',
                                    '%','0','2','x','%','0','2','x','-','%','0','2','x','%','0','2','x',
                                    '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','}',0};
    WCHAR link[sizeof(volumeW)];
    GUID guid;

    memset( &guid, 0, sizeof(guid) );  /* FIXME */
    guid.Data4[7] = 'A' + drive;
    sprintfW( link, volumeW, guid.Data1, guid.Data2, guid.Data3,
              guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return add_mount_point( device, device_name, link, id, id_len );
}

/* check if a given mount point matches the requested specs */
static BOOL matching_mount_point( const struct mount_point *mount, const MOUNTMGR_MOUNT_POINT *spec )
{
    if (spec->SymbolicLinkNameOffset)
    {
        const WCHAR *name = (const WCHAR *)((const char *)spec + spec->SymbolicLinkNameOffset);
        if (spec->SymbolicLinkNameLength != mount->link.Length) return FALSE;
        if (memicmpW( name, mount->link.Buffer, mount->link.Length/sizeof(WCHAR)))
            return FALSE;
    }
    if (spec->DeviceNameOffset)
    {
        const WCHAR *name = (const WCHAR *)((const char *)spec + spec->DeviceNameOffset);
        const UNICODE_STRING *dev_name = get_device_name( mount->device );
        if (spec->DeviceNameLength != dev_name->Length) return FALSE;
        if (memicmpW( name, dev_name->Buffer, dev_name->Length/sizeof(WCHAR)))
            return FALSE;
    }
    if (spec->UniqueIdOffset)
    {
        const void *id = ((const char *)spec + spec->UniqueIdOffset);
        if (spec->UniqueIdLength != mount->id_len) return FALSE;
        if (memcmp( id, mount->id, mount->id_len )) return FALSE;
    }
    return TRUE;
}

/* implementation of IOCTL_MOUNTMGR_QUERY_POINTS */
static NTSTATUS query_mount_points( const void *in_buff, SIZE_T insize,
                                    void *out_buff, SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    UINT count, pos, size;
    const MOUNTMGR_MOUNT_POINT *input = in_buff;
    MOUNTMGR_MOUNT_POINTS *info = out_buff;
    UNICODE_STRING *dev_name;
    struct mount_point *mount;

    /* sanity checks */
    if (input->SymbolicLinkNameOffset + input->SymbolicLinkNameLength > insize ||
        input->UniqueIdOffset + input->UniqueIdLength > insize ||
        input->DeviceNameOffset + input->DeviceNameLength > insize ||
        input->SymbolicLinkNameOffset + input->SymbolicLinkNameLength < input->SymbolicLinkNameOffset ||
        input->UniqueIdOffset + input->UniqueIdLength < input->UniqueIdOffset ||
        input->DeviceNameOffset + input->DeviceNameLength < input->DeviceNameOffset)
        return STATUS_INVALID_PARAMETER;

    count = size = 0;
    LIST_FOR_EACH_ENTRY( mount, &mount_points_list, struct mount_point, entry )
    {
        if (!matching_mount_point( mount, input )) continue;
        size += get_device_name(mount->device)->Length;
        size += mount->link.Length;
        size += mount->id_len;
        size = (size + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
        count++;
    }
    pos = FIELD_OFFSET( MOUNTMGR_MOUNT_POINTS, MountPoints[count] );
    size += pos;

    if (size > outsize)
    {
        if (size >= sizeof(info->Size)) info->Size = size;
        iosb->Information = sizeof(info->Size);
        return STATUS_MORE_ENTRIES;
    }

    info->NumberOfMountPoints = count;
    count = 0;
    LIST_FOR_EACH_ENTRY( mount, &mount_points_list, struct mount_point, entry )
    {
        if (!matching_mount_point( mount, input )) continue;

        dev_name = get_device_name( mount->device );
        info->MountPoints[count].DeviceNameOffset = pos;
        info->MountPoints[count].DeviceNameLength = dev_name->Length;
        memcpy( (char *)out_buff + pos, dev_name->Buffer, dev_name->Length );
        pos += dev_name->Length;

        info->MountPoints[count].SymbolicLinkNameOffset = pos;
        info->MountPoints[count].SymbolicLinkNameLength = mount->link.Length;
        memcpy( (char *)out_buff + pos, mount->link.Buffer, mount->link.Length );
        pos += mount->link.Length;

        info->MountPoints[count].UniqueIdOffset = pos;
        info->MountPoints[count].UniqueIdLength = mount->id_len;
        memcpy( (char *)out_buff + pos, mount->id, mount->id_len );
        pos += mount->id_len;
        pos = (pos + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
        count++;
    }
    info->Size = pos;
    iosb->Information = pos;
    return STATUS_SUCCESS;
}

/* handler for ioctls on the mount manager device */
static NTSTATUS WINAPI mountmgr_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = irp->Tail.Overlay.s.u.CurrentStackLocation;

    TRACE( "ioctl %x insize %u outsize %u\n",
           irpsp->Parameters.DeviceIoControl.IoControlCode,
           irpsp->Parameters.DeviceIoControl.InputBufferLength,
           irpsp->Parameters.DeviceIoControl.OutputBufferLength );

    switch(irpsp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_MOUNTMGR_QUERY_POINTS:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_MOUNT_POINT))
            return STATUS_INVALID_PARAMETER;
        irp->IoStatus.u.Status = query_mount_points( irpsp->Parameters.DeviceIoControl.Type3InputBuffer,
                                                     irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                                     irp->MdlAddress->StartVa,
                                                     irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                                     &irp->IoStatus );
        break;
    default:
        FIXME( "ioctl %x not supported\n", irpsp->Parameters.DeviceIoControl.IoControlCode );
        irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
        break;
    }
    return irp->IoStatus.u.Status;
}

/* handler for ioctls on the harddisk device */
static NTSTATUS WINAPI harddisk_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = irp->Tail.Overlay.s.u.CurrentStackLocation;
    struct disk_device_info *disk_info = device->DeviceExtension;

    TRACE( "ioctl %x insize %u outsize %u\n",
           irpsp->Parameters.DeviceIoControl.IoControlCode,
           irpsp->Parameters.DeviceIoControl.InputBufferLength,
           irpsp->Parameters.DeviceIoControl.OutputBufferLength );

    switch(irpsp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    {
        DISK_GEOMETRY info;
        DWORD len = min( sizeof(info), irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        info.Cylinders.QuadPart = 10000;
        info.MediaType = (disk_info->devnum.DeviceType == FILE_DEVICE_DISK) ? FixedMedia : RemovableMedia;
        info.TracksPerCylinder = 255;
        info.SectorsPerTrack = 63;
        info.BytesPerSector = 512;
        memcpy( irp->MdlAddress->StartVa, &info, len );
        irp->IoStatus.Information = len;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
    {
        DWORD len = min( sizeof(disk_info->devnum), irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        memcpy( irp->MdlAddress->StartVa, &disk_info->devnum, len );
        irp->IoStatus.Information = len;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_CDROM_READ_TOC:
        irp->IoStatus.u.Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    default:
        FIXME( "unsupported ioctl %x\n", irpsp->Parameters.DeviceIoControl.IoControlCode );
        irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
        break;
    }
    return irp->IoStatus.u.Status;
}

/* create mount points for mapped drives */
static void create_drive_mount_points( DRIVER_OBJECT *driver )
{
    const char *config_dir = wine_get_config_dir();
    char *buffer, *p, *link;
    unsigned int i;
    DEVICE_OBJECT *device;

    if ((buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                   strlen(config_dir) + sizeof("/dosdevices/a:") )))
    {
        strcpy( buffer, config_dir );
        strcat( buffer, "/dosdevices/a:" );
        p = buffer + strlen(buffer) - 2;

        for (i = 0; i < MAX_DOS_DRIVES; i++)
        {
            *p = 'a' + i;
            if (!(link = read_symlink( buffer ))) continue;
            if (!create_disk_device( driver, DRIVE_FIXED, &device ))
            {
                add_dosdev_mount_point( device, get_device_name(device), i, link, strlen(link) + 1 );
                add_volume_mount_point( device, get_device_name(device), i, link, strlen(link) + 1 );
            }
            RtlFreeHeap( GetProcessHeap(), 0, link );
        }
        RtlFreeHeap( GetProcessHeap(), 0, buffer );
    }
}

/* driver entry point for the harddisk driver */
static NTSTATUS WINAPI harddisk_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    static const WCHAR mounted_devicesW[] = {'S','y','s','t','e','m','\\',
                                             'M','o','u','n','t','e','d','D','e','v','i','c','e','s',0};
    static const WCHAR harddisk0W[] = {'\\','D','e','v','i','c','e',
                                       '\\','H','a','r','d','d','i','s','k','0',0};
    static const WCHAR physdrive0W[] = {'\\','?','?','\\','P','h','y','s','i','c','a','l','D','r','i','v','e','0',0};

    UNICODE_STRING nameW, linkW;
    DEVICE_OBJECT *device;
    NTSTATUS status;
    struct disk_device_info *info;

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = harddisk_ioctl;

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, mounted_devicesW, 0, NULL,
                     REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &mount_key, NULL );

    RtlInitUnicodeString( &nameW, harddisk0W );
    RtlInitUnicodeString( &linkW, physdrive0W );
    if (!(status = IoCreateDevice( driver, sizeof(*info), &nameW, 0, 0, FALSE, &device )))
        status = IoCreateSymbolicLink( &linkW, &nameW );
    if (status)
    {
        FIXME( "failed to create device error %x\n", status );
        return status;
    }
    info = device->DeviceExtension;
    info->name = nameW;
    info->devnum.DeviceType = FILE_DEVICE_DISK;
    info->devnum.DeviceNumber = 0;
    info->devnum.PartitionNumber = 0;

    create_drive_mount_points( driver );

    return status;
}

/* main entry point for the mount point manager driver */
NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    static const WCHAR device_mountmgrW[] = {'\\','D','e','v','i','c','e','\\','M','o','u','n','t','P','o','i','n','t','M','a','n','a','g','e','r',0};
    static const WCHAR link_mountmgrW[] = {'\\','?','?','\\','M','o','u','n','t','P','o','i','n','t','M','a','n','a','g','e','r',0};
    static const WCHAR harddiskW[] = {'\\','D','r','i','v','e','r','\\','H','a','r','d','d','i','s','k',0};

    UNICODE_STRING nameW, linkW;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    TRACE( "%s\n", debugstr_w(path->Buffer) );

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = mountmgr_ioctl;

    RtlInitUnicodeString( &nameW, device_mountmgrW );
    RtlInitUnicodeString( &linkW, link_mountmgrW );
    if (!(status = IoCreateDevice( driver, 0, &nameW, 0, 0, FALSE, &device )))
        status = IoCreateSymbolicLink( &linkW, &nameW );
    if (status)
    {
        FIXME( "failed to create device error %x\n", status );
        return status;
    }

    initialize_hal();
    initialize_diskarbitration();

    RtlInitUnicodeString( &nameW, harddiskW );
    status = IoCreateDriver( &nameW, harddisk_driver_entry );

    return status;
}
