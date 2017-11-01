/*
 * Dynamic devices support
 *
 * Copyright 2006 Alexandre Julliard
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
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

#define NONAMELESSUNION

#include "mountmgr.h"
#include "winreg.h"
#include "winuser.h"
#include "dbt.h"

#include "wine/library.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#define MAX_DOS_DRIVES 26
#define MAX_PORTS 256

static const WCHAR drive_types[][8] =
{
    { 0 },                           /* DEVICE_UNKNOWN */
    { 0 },                           /* DEVICE_HARDDISK */
    {'h','d',0},                     /* DEVICE_HARDDISK_VOL */
    {'f','l','o','p','p','y',0},     /* DEVICE_FLOPPY */
    {'c','d','r','o','m',0},         /* DEVICE_CDROM */
    {'c','d','r','o','m',0},         /* DEVICE_DVD */
    {'n','e','t','w','o','r','k',0}, /* DEVICE_NETWORK */
    {'r','a','m','d','i','s','k',0}  /* DEVICE_RAMDISK */
};

static const WCHAR drives_keyW[] = {'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e','\\','D','r','i','v','e','s',0};
static const WCHAR ports_keyW[] = {'S','o','f','t','w','a','r','e','\\',
                                   'W','i','n','e','\\','P','o','r','t','s',0};

struct disk_device
{
    enum device_type      type;        /* drive type */
    DEVICE_OBJECT        *dev_obj;     /* disk device allocated for this volume */
    UNICODE_STRING        name;        /* device name */
    UNICODE_STRING        symlink;     /* device symlink if any */
    STORAGE_DEVICE_NUMBER devnum;      /* device number info */
    char                 *unix_device; /* unix device path */
    char                 *unix_mount;  /* unix mount point path */
};

struct volume
{
    struct list           entry;       /* entry in volumes list */
    struct disk_device   *device;      /* disk device */
    char                 *udi;         /* unique identifier for dynamic volumes */
    unsigned int          ref;         /* ref count */
    GUID                  guid;        /* volume uuid */
    struct mount_point   *mount;       /* Volume{xxx} mount point */
};

struct dos_drive
{
    struct list           entry;       /* entry in drives list */
    struct volume        *volume;      /* volume for this drive */
    int                   drive;       /* drive letter (0 = A: etc.) */
    struct mount_point   *mount;       /* DosDevices mount point */
};

static struct list drives_list = LIST_INIT(drives_list);
static struct list volumes_list = LIST_INIT(volumes_list);

static DRIVER_OBJECT *harddisk_driver;
static DRIVER_OBJECT *serial_driver;
static DRIVER_OBJECT *parallel_driver;

static CRITICAL_SECTION device_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &device_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": device_section") }
};
static CRITICAL_SECTION device_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static char *get_dosdevices_path( char **device )
{
    const char *config_dir = wine_get_config_dir();
    size_t len = strlen(config_dir) + sizeof("/dosdevices/com256");
    char *path = HeapAlloc( GetProcessHeap(), 0, len );
    if (path)
    {
        strcpy( path, config_dir );
        strcat( path, "/dosdevices/a::" );
        *device = path + len - sizeof("com256");
    }
    return path;
}

static char *strdupA( const char *str )
{
    char *ret;

    if (!str) return NULL;
    if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, strlen(str) + 1 ))) strcpy( ret, str );
    return ret;
}

static const GUID *get_default_uuid( int letter )
{
    static GUID guid;

    guid.Data4[7] = 'A' + letter;
    return &guid;
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

/* update a symlink if it changed; return TRUE if updated */
static void update_symlink( const char *path, const char *dest, const char *orig_dest )
{
    if (dest && dest[0])
    {
        if (!orig_dest || strcmp( orig_dest, dest ))
        {
            unlink( path );
            symlink( dest, path );
        }
    }
    else unlink( path );
}

/* send notification about a change to a given drive */
static void send_notify( int drive, int code )
{
    DEV_BROADCAST_VOLUME info;

    info.dbcv_size       = sizeof(info);
    info.dbcv_devicetype = DBT_DEVTYP_VOLUME;
    info.dbcv_reserved   = 0;
    info.dbcv_unitmask   = 1 << drive;
    info.dbcv_flags      = DBTF_MEDIA;
    BroadcastSystemMessageW( BSF_FORCEIFHUNG|BSF_QUERY, NULL,
                             WM_DEVICECHANGE, code, (LPARAM)&info );
}

/* create the disk device for a given volume */
static NTSTATUS create_disk_device( enum device_type type, struct disk_device **device_ret )
{
    static const WCHAR harddiskvolW[] = {'\\','D','e','v','i','c','e',
                                         '\\','H','a','r','d','d','i','s','k','V','o','l','u','m','e','%','u',0};
    static const WCHAR harddiskW[] = {'\\','D','e','v','i','c','e','\\','H','a','r','d','d','i','s','k','%','u',0};
    static const WCHAR cdromW[] = {'\\','D','e','v','i','c','e','\\','C','d','R','o','m','%','u',0};
    static const WCHAR floppyW[] = {'\\','D','e','v','i','c','e','\\','F','l','o','p','p','y','%','u',0};
    static const WCHAR ramdiskW[] = {'\\','D','e','v','i','c','e','\\','R','a','m','d','i','s','k','%','u',0};
    static const WCHAR cdromlinkW[] = {'\\','?','?','\\','C','d','R','o','m','%','u',0};
    static const WCHAR physdriveW[] = {'\\','?','?','\\','P','h','y','s','i','c','a','l','D','r','i','v','e','%','u',0};

    UINT i, first = 0;
    NTSTATUS status = 0;
    const WCHAR *format = NULL;
    const WCHAR *link_format = NULL;
    UNICODE_STRING name;
    DEVICE_OBJECT *dev_obj;
    struct disk_device *device;

    switch(type)
    {
    case DEVICE_UNKNOWN:
    case DEVICE_HARDDISK:
    case DEVICE_NETWORK:  /* FIXME */
        format = harddiskW;
        link_format = physdriveW;
        break;
    case DEVICE_HARDDISK_VOL:
        format = harddiskvolW;
        first = 1;  /* harddisk volumes start counting from 1 */
        break;
    case DEVICE_FLOPPY:
        format = floppyW;
        break;
    case DEVICE_CDROM:
    case DEVICE_DVD:
        format = cdromW;
        link_format = cdromlinkW;
        break;
    case DEVICE_RAMDISK:
        format = ramdiskW;
        break;
    }

    name.MaximumLength = (strlenW(format) + 10) * sizeof(WCHAR);
    name.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, name.MaximumLength );
    for (i = first; i < 32; i++)
    {
        sprintfW( name.Buffer, format, i );
        name.Length = strlenW(name.Buffer) * sizeof(WCHAR);
        status = IoCreateDevice( harddisk_driver, sizeof(*device), &name, 0, 0, FALSE, &dev_obj );
        if (status != STATUS_OBJECT_NAME_COLLISION) break;
    }
    if (!status)
    {
        device = dev_obj->DeviceExtension;
        device->dev_obj        = dev_obj;
        device->name           = name;
        device->type           = type;
        device->unix_device    = NULL;
        device->unix_mount     = NULL;
        device->symlink.Buffer = NULL;

        if (link_format)
        {
            UNICODE_STRING symlink;

            symlink.MaximumLength = (strlenW(link_format) + 10) * sizeof(WCHAR);
            if ((symlink.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, symlink.MaximumLength)))
            {
                sprintfW( symlink.Buffer, link_format, i );
                symlink.Length = strlenW(symlink.Buffer) * sizeof(WCHAR);
                if (!IoCreateSymbolicLink( &symlink, &name )) device->symlink = symlink;
            }
        }

        switch (type)
        {
        case DEVICE_FLOPPY:
        case DEVICE_RAMDISK:
            device->devnum.DeviceType = FILE_DEVICE_DISK;
            device->devnum.DeviceNumber = i;
            device->devnum.PartitionNumber = ~0u;
            break;
        case DEVICE_CDROM:
            device->devnum.DeviceType = FILE_DEVICE_CD_ROM;
            device->devnum.DeviceNumber = i;
            device->devnum.PartitionNumber = ~0u;
            break;
        case DEVICE_DVD:
            device->devnum.DeviceType = FILE_DEVICE_DVD;
            device->devnum.DeviceNumber = i;
            device->devnum.PartitionNumber = ~0u;
            break;
        case DEVICE_UNKNOWN:
        case DEVICE_HARDDISK:
        case DEVICE_NETWORK:  /* FIXME */
            device->devnum.DeviceType = FILE_DEVICE_DISK;
            device->devnum.DeviceNumber = i;
            device->devnum.PartitionNumber = 0;
            break;
        case DEVICE_HARDDISK_VOL:
            device->devnum.DeviceType = FILE_DEVICE_DISK;
            device->devnum.DeviceNumber = 0;
            device->devnum.PartitionNumber = i;
            break;
        }
        *device_ret = device;
        TRACE( "created device %s\n", debugstr_w(name.Buffer) );
    }
    else
    {
        FIXME( "IoCreateDevice %s got %x\n", debugstr_w(name.Buffer), status );
        RtlFreeUnicodeString( &name );
    }
    return status;
}

/* delete the disk device for a given drive */
static void delete_disk_device( struct disk_device *device )
{
    TRACE( "deleting device %s\n", debugstr_w(device->name.Buffer) );
    if (device->symlink.Buffer)
    {
        IoDeleteSymbolicLink( &device->symlink );
        RtlFreeUnicodeString( &device->symlink );
    }
    RtlFreeHeap( GetProcessHeap(), 0, device->unix_device );
    RtlFreeHeap( GetProcessHeap(), 0, device->unix_mount );
    RtlFreeUnicodeString( &device->name );
    IoDeleteDevice( device->dev_obj );
}

/* grab another reference to a volume */
static struct volume *grab_volume( struct volume *volume )
{
    volume->ref++;
    return volume;
}

/* release a volume and delete the corresponding disk device when refcount is 0 */
static unsigned int release_volume( struct volume *volume )
{
    unsigned int ret = --volume->ref;

    if (!ret)
    {
        TRACE( "%s udi %s\n", debugstr_guid(&volume->guid), debugstr_a(volume->udi) );
        assert( !volume->udi );
        list_remove( &volume->entry );
        if (volume->mount) delete_mount_point( volume->mount );
        delete_disk_device( volume->device );
        RtlFreeHeap( GetProcessHeap(), 0, volume );
    }
    return ret;
}

/* set the volume udi */
static void set_volume_udi( struct volume *volume, const char *udi )
{
    if (udi)
    {
        assert( !volume->udi );
        /* having a udi means the HAL side holds an extra reference */
        if ((volume->udi = strdupA( udi ))) grab_volume( volume );
    }
    else if (volume->udi)
    {
        RtlFreeHeap( GetProcessHeap(), 0, volume->udi );
        volume->udi = NULL;
        release_volume( volume );
    }
}

/* create a disk volume */
static NTSTATUS create_volume( const char *udi, enum device_type type, struct volume **volume_ret )
{
    struct volume *volume;
    NTSTATUS status;

    if (!(volume = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*volume) )))
        return STATUS_NO_MEMORY;

    if (!(status = create_disk_device( type, &volume->device )))
    {
        if (udi) set_volume_udi( volume, udi );
        list_add_tail( &volumes_list, &volume->entry );
        *volume_ret = grab_volume( volume );
    }
    else RtlFreeHeap( GetProcessHeap(), 0, volume );

    return status;
}

/* create the disk device for a given volume */
static NTSTATUS create_dos_device( struct volume *volume, const char *udi, int letter,
                                   enum device_type type, struct dos_drive **drive_ret )
{
    struct dos_drive *drive;
    NTSTATUS status;

    if (!(drive = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*drive) ))) return STATUS_NO_MEMORY;
    drive->drive = letter;
    drive->mount = NULL;

    if (volume)
    {
        if (udi) set_volume_udi( volume, udi );
        drive->volume = grab_volume( volume );
        status = STATUS_SUCCESS;
    }
    else status = create_volume( udi, type, &drive->volume );

    if (status == STATUS_SUCCESS)
    {
        list_add_tail( &drives_list, &drive->entry );
        *drive_ret = drive;
    }
    else RtlFreeHeap( GetProcessHeap(), 0, drive );

    return status;
}

/* delete the disk device for a given drive */
static void delete_dos_device( struct dos_drive *drive )
{
    list_remove( &drive->entry );
    if (drive->mount) delete_mount_point( drive->mount );
    release_volume( drive->volume );
    RtlFreeHeap( GetProcessHeap(), 0, drive );
}

/* find a volume that matches the parameters */
static struct volume *find_matching_volume( const char *udi, const char *device,
                                            const char *mount_point, enum device_type type )
{
    struct volume *volume;
    struct disk_device *disk_device;

    LIST_FOR_EACH_ENTRY( volume, &volumes_list, struct volume, entry )
    {
        int match = 0;

        /* when we have a udi we only match drives added manually */
        if (udi && volume->udi) continue;
        /* and when we don't have a udi we only match dynamic drives */
        if (!udi && !volume->udi) continue;

        disk_device = volume->device;
        if (disk_device->type != type) continue;
        if (device && disk_device->unix_device)
        {
            if (strcmp( device, disk_device->unix_device )) continue;
            match++;
        }
        if (mount_point && disk_device->unix_mount)
        {
            if (strcmp( mount_point, disk_device->unix_mount )) continue;
            match++;
        }
        if (!match) continue;
        TRACE( "found matching volume %s for device %s mount %s type %u\n",
               debugstr_guid(&volume->guid), debugstr_a(device), debugstr_a(mount_point), type );
        return grab_volume( volume );
    }
    return NULL;
}

/* change the information for an existing volume */
static NTSTATUS set_volume_info( struct volume *volume, struct dos_drive *drive, const char *device,
                                 const char *mount_point, enum device_type type, const GUID *guid )
{
    void *id = NULL;
    unsigned int id_len = 0;
    struct disk_device *disk_device = volume->device;
    NTSTATUS status;

    if (type != disk_device->type)
    {
        if ((status = create_disk_device( type, &disk_device ))) return status;
        if (volume->mount)
        {
            delete_mount_point( volume->mount );
            volume->mount = NULL;
        }
        if (drive && drive->mount)
        {
            delete_mount_point( drive->mount );
            drive->mount = NULL;
        }
        delete_disk_device( volume->device );
        volume->device = disk_device;
    }
    else
    {
        RtlFreeHeap( GetProcessHeap(), 0, disk_device->unix_device );
        RtlFreeHeap( GetProcessHeap(), 0, disk_device->unix_mount );
    }
    disk_device->unix_device = strdupA( device );
    disk_device->unix_mount = strdupA( mount_point );

    if (guid && memcmp( &volume->guid, guid, sizeof(volume->guid) ))
    {
        volume->guid = *guid;
        if (volume->mount)
        {
            delete_mount_point( volume->mount );
            volume->mount = NULL;
        }
    }

    if (!volume->mount)
        volume->mount = add_volume_mount_point( disk_device->dev_obj, &disk_device->name, &volume->guid );
    if (drive && !drive->mount)
        drive->mount = add_dosdev_mount_point( disk_device->dev_obj, &disk_device->name, drive->drive );

    if (disk_device->unix_mount)
    {
        id = disk_device->unix_mount;
        id_len = strlen( disk_device->unix_mount ) + 1;
    }
    if (volume->mount) set_mount_point_id( volume->mount, id, id_len );
    if (drive && drive->mount) set_mount_point_id( drive->mount, id, id_len );

    return STATUS_SUCCESS;
}

/* change the drive letter or volume for an existing drive */
static void set_drive_info( struct dos_drive *drive, int letter, struct volume *volume )
{
    if (drive->drive != letter)
    {
        if (drive->mount) delete_mount_point( drive->mount );
        drive->mount = NULL;
        drive->drive = letter;
    }
    if (drive->volume != volume)
    {
        if (drive->mount) delete_mount_point( drive->mount );
        drive->mount = NULL;
        grab_volume( volume );
        release_volume( drive->volume );
        drive->volume = volume;
    }
}

static inline BOOL is_valid_device( struct stat *st )
{
#if defined(linux) || defined(__sun__)
    return S_ISBLK( st->st_mode );
#else
    /* disks are char devices on *BSD */
    return S_ISCHR( st->st_mode );
#endif
}

/* find or create a DOS drive for the corresponding device */
static int add_drive( const char *device, enum device_type type )
{
    char *path, *p;
    char in_use[26];
    struct stat dev_st, drive_st;
    int drive, first, last, avail = 0;

    if (stat( device, &dev_st ) == -1 || !is_valid_device( &dev_st )) return -1;

    if (!(path = get_dosdevices_path( &p ))) return -1;

    memset( in_use, 0, sizeof(in_use) );

    switch (type)
    {
    case DEVICE_FLOPPY:
        first = 0;
        last = 2;
        break;
    case DEVICE_CDROM:
    case DEVICE_DVD:
        first = 3;
        last = 26;
        break;
    default:
        first = 2;
        last = 26;
        break;
    }

    while (avail != -1)
    {
        avail = -1;
        for (drive = first; drive < last; drive++)
        {
            if (in_use[drive]) continue;  /* already checked */
            *p = 'a' + drive;
            if (stat( path, &drive_st ) == -1)
            {
                if (lstat( path, &drive_st ) == -1 && errno == ENOENT)  /* this is a candidate */
                {
                    if (avail == -1)
                    {
                        p[2] = 0;
                        /* if mount point symlink doesn't exist either, it's available */
                        if (lstat( path, &drive_st ) == -1 && errno == ENOENT) avail = drive;
                        p[2] = ':';
                    }
                }
                else in_use[drive] = 1;
            }
            else
            {
                in_use[drive] = 1;
                if (!is_valid_device( &drive_st )) continue;
                if (dev_st.st_rdev == drive_st.st_rdev) goto done;
            }
        }
        if (avail != -1)
        {
            /* try to use the one we found */
            drive = avail;
            *p = 'a' + drive;
            if (symlink( device, path ) != -1) goto done;
            /* failed, retry the search */
        }
    }
    drive = -1;

done:
    HeapFree( GetProcessHeap(), 0, path );
    return drive;
}

/* create devices for mapped drives */
static void create_drive_devices(void)
{
    char *path, *p, *link, *device;
    struct dos_drive *drive;
    struct volume *volume;
    unsigned int i;
    HKEY drives_key;
    enum device_type drive_type;
    WCHAR driveW[] = {'a',':',0};

    if (!(path = get_dosdevices_path( &p ))) return;
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, drives_keyW, &drives_key )) drives_key = 0;

    for (i = 0; i < MAX_DOS_DRIVES; i++)
    {
        p[0] = 'a' + i;
        p[2] = 0;
        if (!(link = read_symlink( path ))) continue;
        p[2] = ':';
        device = read_symlink( path );

        drive_type = i < 2 ? DEVICE_FLOPPY : DEVICE_HARDDISK_VOL;
        if (drives_key)
        {
            WCHAR buffer[32];
            DWORD j, type, size = sizeof(buffer);

            driveW[0] = 'a' + i;
            if (!RegQueryValueExW( drives_key, driveW, NULL, &type, (BYTE *)buffer, &size ) &&
                type == REG_SZ)
            {
                for (j = 0; j < sizeof(drive_types)/sizeof(drive_types[0]); j++)
                    if (drive_types[j][0] && !strcmpiW( buffer, drive_types[j] ))
                    {
                        drive_type = j;
                        break;
                    }
                if (drive_type == DEVICE_FLOPPY && i >= 2) drive_type = DEVICE_HARDDISK;
            }
        }

        volume = find_matching_volume( NULL, device, link, drive_type );
        if (!create_dos_device( volume, NULL, i, drive_type, &drive ))
        {
            /* don't reset uuid if we used an existing volume */
            const GUID *guid = volume ? NULL : get_default_uuid(i);
            set_volume_info( drive->volume, drive, device, link, drive_type, guid );
        }
        else
        {
            RtlFreeHeap( GetProcessHeap(), 0, link );
            RtlFreeHeap( GetProcessHeap(), 0, device );
        }
        if (volume) release_volume( volume );
    }
    RegCloseKey( drives_key );
    RtlFreeHeap( GetProcessHeap(), 0, path );
}

/* create a new disk volume */
NTSTATUS add_volume( const char *udi, const char *device, const char *mount_point,
                     enum device_type type, const GUID *guid )
{
    struct volume *volume;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "adding %s device %s mount %s type %u uuid %s\n", debugstr_a(udi),
           debugstr_a(device), debugstr_a(mount_point), type, debugstr_guid(guid) );

    EnterCriticalSection( &device_section );
    LIST_FOR_EACH_ENTRY( volume, &volumes_list, struct volume, entry )
        if (volume->udi && !strcmp( udi, volume->udi ))
        {
            grab_volume( volume );
            goto found;
        }

    /* udi not found, search for a non-dynamic volume */
    if ((volume = find_matching_volume( udi, device, mount_point, type ))) set_volume_udi( volume, udi );
    else status = create_volume( udi, type, &volume );

found:
    if (!status) status = set_volume_info( volume, NULL, device, mount_point, type, guid );
    if (volume) release_volume( volume );
    LeaveCriticalSection( &device_section );
    return status;
}

/* create a new disk volume */
NTSTATUS remove_volume( const char *udi )
{
    NTSTATUS status = STATUS_NO_SUCH_DEVICE;
    struct volume *volume;

    EnterCriticalSection( &device_section );
    LIST_FOR_EACH_ENTRY( volume, &volumes_list, struct volume, entry )
    {
        if (!volume->udi || strcmp( udi, volume->udi )) continue;
        set_volume_udi( volume, NULL );
        status = STATUS_SUCCESS;
        break;
    }
    LeaveCriticalSection( &device_section );
    return status;
}


/* create a new dos drive */
NTSTATUS add_dos_device( int letter, const char *udi, const char *device,
                         const char *mount_point, enum device_type type, const GUID *guid )
{
    char *path, *p;
    HKEY hkey;
    NTSTATUS status = STATUS_SUCCESS;
    struct dos_drive *drive, *next;
    struct volume *volume;
    int notify = -1;

    if (!(path = get_dosdevices_path( &p ))) return STATUS_NO_MEMORY;

    EnterCriticalSection( &device_section );
    volume = find_matching_volume( udi, device, mount_point, type );

    if (letter == -1)  /* auto-assign a letter */
    {
        letter = add_drive( device, type );
        if (letter == -1)
        {
            status = STATUS_OBJECT_NAME_COLLISION;
            goto done;
        }

        LIST_FOR_EACH_ENTRY_SAFE( drive, next, &drives_list, struct dos_drive, entry )
        {
            if (drive->volume->udi && !strcmp( udi, drive->volume->udi )) goto found;
            if (drive->drive == letter) delete_dos_device( drive );
        }
    }
    else  /* simply reset the device symlink */
    {
        LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
            if (drive->drive == letter) break;

        *p = 'a' + letter;
        if (&drive->entry == &drives_list) update_symlink( path, device, NULL );
        else
        {
            update_symlink( path, device, drive->volume->device->unix_device );
            delete_dos_device( drive );
        }
    }

    if ((status = create_dos_device( volume, udi, letter, type, &drive ))) goto done;

found:
    if (!guid && !volume) guid = get_default_uuid( letter );
    if (!volume) volume = grab_volume( drive->volume );
    set_drive_info( drive, letter, volume );
    p[0] = 'a' + drive->drive;
    p[2] = 0;
    update_symlink( path, mount_point, volume->device->unix_mount );
    set_volume_info( volume, drive, device, mount_point, type, guid );

    TRACE( "added device %c: udi %s for %s on %s type %u\n",
           'a' + drive->drive, wine_dbgstr_a(udi), wine_dbgstr_a(device),
           wine_dbgstr_a(mount_point), type );

    /* hack: force the drive type in the registry */
    if (!RegCreateKeyW( HKEY_LOCAL_MACHINE, drives_keyW, &hkey ))
    {
        const WCHAR *type_name = drive_types[type];
        WCHAR name[] = {'a',':',0};

        name[0] += drive->drive;
        if (!type_name[0] && type == DEVICE_HARDDISK) type_name = drive_types[DEVICE_FLOPPY];
        if (type_name[0])
            RegSetValueExW( hkey, name, 0, REG_SZ, (const BYTE *)type_name,
                            (strlenW(type_name) + 1) * sizeof(WCHAR) );
        else
            RegDeleteValueW( hkey, name );
        RegCloseKey( hkey );
    }

    if (udi) notify = drive->drive;

done:
    if (volume) release_volume( volume );
    LeaveCriticalSection( &device_section );
    RtlFreeHeap( GetProcessHeap(), 0, path );
    if (notify != -1) send_notify( notify, DBT_DEVICEARRIVAL );
    return status;
}

/* remove an existing dos drive, by letter or udi */
NTSTATUS remove_dos_device( int letter, const char *udi )
{
    NTSTATUS status = STATUS_NO_SUCH_DEVICE;
    HKEY hkey;
    struct dos_drive *drive;
    char *path, *p;
    int notify = -1;

    EnterCriticalSection( &device_section );
    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        if (udi)
        {
            if (!drive->volume->udi) continue;
            if (strcmp( udi, drive->volume->udi )) continue;
            set_volume_udi( drive->volume, NULL );
        }
        else if (drive->drive != letter) continue;

        if ((path = get_dosdevices_path( &p )))
        {
            p[0] = 'a' + drive->drive;
            p[2] = 0;
            unlink( path );
            RtlFreeHeap( GetProcessHeap(), 0, path );
        }

        /* clear the registry key too */
        if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, drives_keyW, &hkey ))
        {
            WCHAR name[] = {'a',':',0};
            name[0] += drive->drive;
            RegDeleteValueW( hkey, name );
            RegCloseKey( hkey );
        }

        if (udi && drive->volume->device->unix_mount) notify = drive->drive;

        delete_dos_device( drive );
        status = STATUS_SUCCESS;
        break;
    }
    LeaveCriticalSection( &device_section );
    if (notify != -1) send_notify( notify, DBT_DEVICEREMOVECOMPLETE );
    return status;
}

/* query information about an existing dos drive, by letter or udi */
NTSTATUS query_dos_device( int letter, enum device_type *type, char **device, char **mount_point )
{
    NTSTATUS status = STATUS_NO_SUCH_DEVICE;
    struct dos_drive *drive;
    struct disk_device *disk_device;

    EnterCriticalSection( &device_section );
    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        if (drive->drive != letter) continue;
        disk_device = drive->volume->device;
        if (type) *type = disk_device->type;
        if (device) *device = strdupA( disk_device->unix_device );
        if (mount_point) *mount_point = strdupA( disk_device->unix_mount );
        status = STATUS_SUCCESS;
        break;
    }
    LeaveCriticalSection( &device_section );
    return status;
}

/* handler for ioctls on the harddisk device */
static NTSTATUS WINAPI harddisk_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    struct disk_device *dev = device->DeviceExtension;

    TRACE( "ioctl %x insize %u outsize %u\n",
           irpsp->Parameters.DeviceIoControl.IoControlCode,
           irpsp->Parameters.DeviceIoControl.InputBufferLength,
           irpsp->Parameters.DeviceIoControl.OutputBufferLength );

    EnterCriticalSection( &device_section );

    switch(irpsp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    {
        DISK_GEOMETRY info;
        DWORD len = min( sizeof(info), irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        info.Cylinders.QuadPart = 10000;
        info.MediaType = (dev->devnum.DeviceType == FILE_DEVICE_DISK) ? FixedMedia : RemovableMedia;
        info.TracksPerCylinder = 255;
        info.SectorsPerTrack = 63;
        info.BytesPerSector = 512;
        memcpy( irp->AssociatedIrp.SystemBuffer, &info, len );
        irp->IoStatus.Information = len;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
    {
        DISK_GEOMETRY_EX info;
        DWORD len = min( sizeof(info), irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        FIXME("The DISK_PARTITION_INFO and DISK_DETECTION_INFO structures will not be filled\n");

        info.Geometry.Cylinders.QuadPart = 10000;
        info.Geometry.MediaType = (dev->devnum.DeviceType == FILE_DEVICE_DISK) ? FixedMedia : RemovableMedia;
        info.Geometry.TracksPerCylinder = 255;
        info.Geometry.SectorsPerTrack = 63;
        info.Geometry.BytesPerSector = 512;
        info.DiskSize.QuadPart = info.Geometry.Cylinders.QuadPart * info.Geometry.TracksPerCylinder *
                                 info.Geometry.SectorsPerTrack * info.Geometry.BytesPerSector;
        info.Data[0]  = 0;
        memcpy( irp->AssociatedIrp.SystemBuffer, &info, len );
        irp->IoStatus.Information = len;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
    {
        DWORD len = min( sizeof(dev->devnum), irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        memcpy( irp->AssociatedIrp.SystemBuffer, &dev->devnum, len );
        irp->IoStatus.Information = len;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_CDROM_READ_TOC:
        irp->IoStatus.u.Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
    {
        DWORD len = min( 32, irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        FIXME( "returning zero-filled buffer for IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS\n" );
        memset( irp->AssociatedIrp.SystemBuffer, 0, len );
        irp->IoStatus.Information = len;
        irp->IoStatus.u.Status = STATUS_SUCCESS;
        break;
    }
    default:
    {
        ULONG code = irpsp->Parameters.DeviceIoControl.IoControlCode;
        FIXME("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
        irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
        break;
    }
    }

    LeaveCriticalSection( &device_section );
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

/* driver entry point for the harddisk driver */
NTSTATUS WINAPI harddisk_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    struct disk_device *device;

    harddisk_driver = driver;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = harddisk_ioctl;

    /* create a harddisk0 device that isn't assigned to any drive */
    create_disk_device( DEVICE_HARDDISK, &device );

    create_drive_devices();

    return STATUS_SUCCESS;
}


/* create a serial or parallel port */
static BOOL create_port_device( DRIVER_OBJECT *driver, int n, const char *unix_path, const char *dosdevices_path, char *p,
                                HKEY wine_ports_key, HKEY windows_ports_key )
{
    static const WCHAR comW[] = {'C','O','M','%','u',0};
    static const WCHAR lptW[] = {'L','P','T','%','u',0};
    static const WCHAR device_serialW[] = {'\\','D','e','v','i','c','e','\\','S','e','r','i','a','l','%','u',0};
    static const WCHAR device_parallelW[] = {'\\','D','e','v','i','c','e','\\','P','a','r','a','l','l','e','l','%','u',0};
    static const WCHAR dosdevices_comW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','C','O','M','%','u',0};
    static const WCHAR dosdevices_auxW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','A','U','X',0};
    static const WCHAR dosdevices_lptW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','L','P','T','%','u',0};
    static const WCHAR dosdevices_prnW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','P','R','N',0};
    const WCHAR *dos_name_format, *nt_name_format, *reg_value_format, *symlink_format, *default_device;
    WCHAR dos_name[7], reg_value[256], nt_buffer[32], symlink_buffer[32];
    DWORD type, size;
    char override_path[256];
    UNICODE_STRING nt_name, symlink_name, default_name;
    DEVICE_OBJECT *dev_obj;
    NTSTATUS status;

    if (driver == serial_driver)
    {
        dos_name_format = comW;
        nt_name_format = device_serialW;
        reg_value_format = comW;
        symlink_format = dosdevices_comW;
        default_device = dosdevices_auxW;
    }
    else
    {
        dos_name_format = lptW;
        nt_name_format = device_parallelW;
        reg_value_format = dosdevices_lptW;
        symlink_format = dosdevices_lptW;
        default_device = dosdevices_prnW;
    }

    sprintfW( dos_name, dos_name_format, n );

    /* check for override */
    size = sizeof(reg_value);
    if (RegQueryValueExW( wine_ports_key, dos_name, NULL, &type, (BYTE *)reg_value, &size ) == 0 && type == REG_SZ)
    {
        if (!reg_value[0] || !WideCharToMultiByte( CP_UNIXCP, WC_ERR_INVALID_CHARS, reg_value, size/sizeof(WCHAR),
                                                   override_path, sizeof(override_path), NULL, NULL))
            return FALSE;
        unix_path = override_path;
    }
    if (!unix_path)
        return FALSE;

    /* create DOS device */
    sprintf( p, "%u", n );
    if (symlink( unix_path, dosdevices_path ) != 0)
        return FALSE;

    /* create NT device */
    sprintfW( nt_buffer, nt_name_format, n - 1 );
    RtlInitUnicodeString( &nt_name, nt_buffer );
    status = IoCreateDevice( driver, 0, &nt_name, 0, 0, FALSE, &dev_obj );
    if (status != STATUS_SUCCESS)
    {
        FIXME( "IoCreateDevice %s got %x\n", debugstr_w(nt_name.Buffer), status );
        return FALSE;
    }
    sprintfW( symlink_buffer, symlink_format, n );
    RtlInitUnicodeString( &symlink_name, symlink_buffer );
    IoCreateSymbolicLink( &symlink_name, &nt_name );
    if (n == 1)
    {
        RtlInitUnicodeString( &default_name, default_device );
        IoCreateSymbolicLink( &default_name, &symlink_name );
    }

    /* TODO: store information about the Unix device in the NT device */

    /* create registry entry */
    sprintfW( reg_value, reg_value_format, n );
    RegSetValueExW( windows_ports_key, nt_name.Buffer, 0, REG_SZ,
                    (BYTE *)reg_value, (strlenW( reg_value ) + 1) * sizeof(WCHAR) );

    return TRUE;
}

/* find and create serial or parallel ports */
static void create_port_devices( DRIVER_OBJECT *driver )
{
    static const char *serial_search_paths[] = {
#ifdef linux
        "/dev/ttyS%u",
        "/dev/ttyUSB%u",
        "/dev/ttyACM%u",
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
        "/dev/cuau%u",
#elif defined(__DragonFly__)
        "/dev/cuaa%u",
#else
        "",
#endif
    };
    static const char *parallel_search_paths[] = {
#ifdef linux
        "/dev/lp%u",
#else
        "",
#endif
    };
    static const WCHAR serialcomm_keyW[] = {'H','A','R','D','W','A','R','E','\\',
                                            'D','E','V','I','C','E','M','A','P','\\',
                                            'S','E','R','I','A','L','C','O','M','M',0};
    static const WCHAR parallel_ports_keyW[] = {'H','A','R','D','W','A','R','E','\\',
                                                'D','E','V','I','C','E','M','A','P','\\',
                                                'P','A','R','A','L','L','E','L',' ','P','O','R','T','S',0};
    const char **search_paths;
    const WCHAR *windows_ports_key_name;
    char *dosdevices_path, *p;
    HKEY wine_ports_key = NULL, windows_ports_key = NULL;
    char unix_path[256];
    int num_search_paths, i, j, n;

    if (!(dosdevices_path = get_dosdevices_path( &p )))
        return;

    if (driver == serial_driver)
    {
        p[0] = 'c';
        p[1] = 'o';
        p[2] = 'm';
        search_paths = serial_search_paths;
        num_search_paths = sizeof(serial_search_paths)/sizeof(serial_search_paths[0]);
        windows_ports_key_name = serialcomm_keyW;
    }
    else
    {
        p[0] = 'l';
        p[1] = 'p';
        p[2] = 't';
        search_paths = parallel_search_paths;
        num_search_paths = sizeof(parallel_search_paths)/sizeof(parallel_search_paths[0]);
        windows_ports_key_name = parallel_ports_keyW;
    }
    p += 3;

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, ports_keyW, 0, NULL, 0,
                     KEY_QUERY_VALUE, NULL, &wine_ports_key, NULL );
    RegCreateKeyExW( HKEY_LOCAL_MACHINE, windows_ports_key_name, 0, NULL, REG_OPTION_VOLATILE,
                     KEY_ALL_ACCESS, NULL, &windows_ports_key, NULL );

    /* remove old symlinks */
    for (n = 1; n <= MAX_PORTS; n++)
    {
        sprintf( p, "%u", n );
        if (unlink( dosdevices_path ) != 0 && errno == ENOENT)
            break;
    }

    /* look for ports in the usual places */
    n = 1;
    for (i = 0; i < num_search_paths; i++)
    {
        for (j = 0; n <= MAX_PORTS; j++)
        {
            sprintf( unix_path, search_paths[i], j );
            if (access( unix_path, F_OK ) != 0)
                break;

            create_port_device( driver, n, unix_path, dosdevices_path, p, wine_ports_key, windows_ports_key );
            n++;
        }
    }

    /* add any extra user-defined serial ports */
    while (n <= MAX_PORTS)
    {
        if (!create_port_device( driver, n, NULL, dosdevices_path, p, wine_ports_key, windows_ports_key ))
            break;
        n++;
    }

    RegCloseKey( wine_ports_key );
    RegCloseKey( windows_ports_key );
    HeapFree( GetProcessHeap(), 0, dosdevices_path );
}

/* driver entry point for the serial port driver */
NTSTATUS WINAPI serial_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    serial_driver = driver;
    /* TODO: fill in driver->MajorFunction */

    create_port_devices( driver );

    return STATUS_SUCCESS;
}

/* driver entry point for the parallel port driver */
NTSTATUS WINAPI parallel_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    parallel_driver = driver;
    /* TODO: fill in driver->MajorFunction */

    create_port_devices( driver );

    return STATUS_SUCCESS;
}
