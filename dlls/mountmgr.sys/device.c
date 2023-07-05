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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "mountmgr.h"
#include "winreg.h"
#include "winnls.h"
#include "winuser.h"
#include "dbt.h"
#include "unixlib.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#define MAX_DOS_DRIVES 26
#define MAX_PORTS 256

static const WCHAR drive_types[][8] =
{
    L"",        /* DEVICE_UNKNOWN */
    L"",        /* DEVICE_HARDDISK */
    L"hd",      /* DEVICE_HARDDISK_VOL */
    L"floppy",  /* DEVICE_FLOPPY */
    L"cdrom",   /* DEVICE_CDROM */
    L"cdrom",   /* DEVICE_DVD */
    L"network", /* DEVICE_NETWORK */
    L"ramdisk"  /* DEVICE_RAMDISK */
};

enum fs_type
{
    FS_ERROR,    /* error accessing the device */
    FS_UNKNOWN,  /* unknown file system */
    FS_FAT1216,
    FS_FAT32,
    FS_ISO9660,
    FS_UDF       /* For reference [E] = Ecma-167.pdf, [U] = udf260.pdf */
};

struct disk_device
{
    enum device_type      type;        /* drive type */
    DEVICE_OBJECT        *dev_obj;     /* disk device allocated for this volume */
    UNICODE_STRING        name;        /* device name */
    UNICODE_STRING        symlink;     /* device symlink if any */
    STORAGE_DEVICE_NUMBER devnum;      /* device number info */
    char                 *unix_device; /* unix device path */
    char                 *unix_mount;  /* unix mount point path */
    char                 *serial;      /* disk serial number */
    struct volume        *volume;      /* associated volume */
};

struct volume
{
    struct list           entry;       /* entry in volumes list */
    struct disk_device   *device;      /* disk device */
    char                 *udi;         /* unique identifier for dynamic volumes */
    unsigned int          ref;         /* ref count */
    GUID                  guid;        /* volume uuid */
    struct mount_point   *mount;       /* Volume{xxx} mount point */
    WCHAR                 label[256];  /* volume label */
    DWORD                 serial;      /* volume serial number */
    enum fs_type          fs_type;     /* file system type */
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

static const GUID *get_default_uuid( int letter )
{
    static GUID guid;

    guid.Data4[7] = 'A' + letter;
    return &guid;
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

#define BLOCK_SIZE 2048
#define SUPERBLOCK_SIZE BLOCK_SIZE

#define CDFRAMES_PERSEC         75
#define CDFRAMES_PERMIN         (CDFRAMES_PERSEC * 60)
#define FRAME_OF_ADDR(a)        ((a)[1] * CDFRAMES_PERMIN + (a)[2] * CDFRAMES_PERSEC + (a)[3])
#define FRAME_OF_TOC(toc, idx)  FRAME_OF_ADDR((toc)->TrackData[(idx) - (toc)->FirstTrack].Address)

#define GETWORD(buf,off)  MAKEWORD(buf[(off)],buf[(off+1)])
#define GETLONG(buf,off)  MAKELONG(GETWORD(buf,off),GETWORD(buf,off+2))

/* get the label by reading it from a file at the root of the filesystem */
static void get_filesystem_label( struct volume *volume )
{
    char buffer[256], *p;
    ULONG size = sizeof(buffer);
    struct read_volume_file_params params = { volume->device->unix_mount, ".windows-label", buffer, &size };

    volume->label[0] = 0;
    if (!volume->device->unix_mount) return;
    if (MOUNTMGR_CALL( read_volume_file, &params )) return;

    p = buffer + size;
    while (p > buffer && (p[-1] == ' ' || p[-1] == '\r' || p[-1] == '\n')) p--;
    *p = 0;
    if (!MultiByteToWideChar( CP_UNIXCP, 0, buffer, -1, volume->label, ARRAY_SIZE(volume->label) ))
        volume->label[ARRAY_SIZE(volume->label) - 1] = 0;
}

/* get the serial number by reading it from a file at the root of the filesystem */
static void get_filesystem_serial( struct volume *volume )
{
    char buffer[32];
    ULONG size = sizeof(buffer);
    struct read_volume_file_params params = { volume->device->unix_mount, ".windows-serial", buffer, &size };

    volume->serial = 0;
    if (!volume->device->unix_mount) return;
    if (MOUNTMGR_CALL( read_volume_file, &params )) return;

    buffer[size] = 0;
    volume->serial = strtoul( buffer, NULL, 16 );
}


/******************************************************************
 *		VOLUME_FindCdRomDataBestVoldesc
 */
static DWORD VOLUME_FindCdRomDataBestVoldesc( HANDLE handle )
{
    BYTE cur_vd_type, max_vd_type = 0;
    BYTE buffer[0x800];
    DWORD size, offs, best_offs = 0, extra_offs = 0;

    for (offs = 0x8000; offs <= 0x9800; offs += 0x800)
    {
        /* if 'CDROM' occurs at position 8, this is a pre-iso9660 cd, and
         * the volume label is displaced forward by 8
         */
        if (SetFilePointer( handle, offs, NULL, FILE_BEGIN ) != offs) break;
        if (!ReadFile( handle, buffer, sizeof(buffer), &size, NULL )) break;
        if (size != sizeof(buffer)) break;
        /* check for non-ISO9660 signature */
        if (!memcmp( buffer + 11, "ROM", 3 )) extra_offs = 8;
        cur_vd_type = buffer[extra_offs];
        if (cur_vd_type == 0xff) /* voldesc set terminator */
            break;
        if (cur_vd_type > max_vd_type)
        {
            max_vd_type = cur_vd_type;
            best_offs = offs + extra_offs;
        }
    }
    return best_offs;
}


/***********************************************************************
 *           VOLUME_ReadFATSuperblock
 */
static enum fs_type VOLUME_ReadFATSuperblock( HANDLE handle, BYTE *buff )
{
    DWORD size;

    /* try a fixed disk, with a FAT partition */
    if (SetFilePointer( handle, 0, NULL, FILE_BEGIN ) != 0 ||
        !ReadFile( handle, buff, SUPERBLOCK_SIZE, &size, NULL ))
    {
        if (GetLastError() == ERROR_BAD_DEV_TYPE) return FS_UNKNOWN;  /* not a real device */
        return FS_ERROR;
    }

    if (size < SUPERBLOCK_SIZE) return FS_UNKNOWN;

    /* FIXME: do really all FAT have their name beginning with
     * "FAT" ? (At least FAT12, FAT16 and FAT32 have :)
     */
    if (!memcmp(buff+0x36, "FAT", 3) || !memcmp(buff+0x52, "FAT", 3))
    {
        /* guess which type of FAT we have */
        int reasonable;
        unsigned int sectors,
                     sect_per_fat,
                     total_sectors,
                     num_boot_sectors,
                     num_fats,
                     num_root_dir_ents,
                     bytes_per_sector,
                     sectors_per_cluster,
                     nclust;
        sect_per_fat = GETWORD(buff, 0x16);
        if (!sect_per_fat) sect_per_fat = GETLONG(buff, 0x24);
        total_sectors = GETWORD(buff, 0x13);
        if (!total_sectors)
            total_sectors = GETLONG(buff, 0x20);
        num_boot_sectors = GETWORD(buff, 0x0e);
        num_fats =  buff[0x10];
        num_root_dir_ents = GETWORD(buff, 0x11);
        bytes_per_sector = GETWORD(buff, 0x0b);
        sectors_per_cluster = buff[0x0d];
        /* check if the parameters are reasonable and will not cause
         * arithmetic errors in the calculation */
        reasonable = num_boot_sectors < total_sectors &&
                     num_fats < 16 &&
                     bytes_per_sector >= 512 && bytes_per_sector % 512 == 0 &&
                     sectors_per_cluster >= 1;
        if (!reasonable) return FS_UNKNOWN;
        sectors =  total_sectors - num_boot_sectors - num_fats * sect_per_fat -
            (num_root_dir_ents * 32 + bytes_per_sector - 1) / bytes_per_sector;
        nclust = sectors / sectors_per_cluster;
        if ((buff[0x42] == 0x28 || buff[0x42] == 0x29) &&
                !memcmp(buff+0x52, "FAT", 3)) return FS_FAT32;
        if (nclust < 65525)
        {
            if ((buff[0x26] == 0x28 || buff[0x26] == 0x29) &&
                    !memcmp(buff+0x36, "FAT", 3))
                return FS_FAT1216;
        }
    }
    return FS_UNKNOWN;
}


/***********************************************************************
 *           VOLUME_ReadCDBlock
 */
static BOOL VOLUME_ReadCDBlock( HANDLE handle, BYTE *buff, INT offs )
{
    DWORD size, whence = offs >= 0 ? FILE_BEGIN : FILE_END;

    if (SetFilePointer( handle, offs, NULL, whence ) != offs ||
        !ReadFile( handle, buff, SUPERBLOCK_SIZE, &size, NULL ) ||
        size != SUPERBLOCK_SIZE)
        return FALSE;

    return TRUE;
}


/***********************************************************************
 *           VOLUME_ReadCDSuperblock
 */
static enum fs_type VOLUME_ReadCDSuperblock( HANDLE handle, BYTE *buff )
{
    int i;
    DWORD offs;

    /* Check UDF first as UDF and ISO9660 structures can coexist on the same medium
     *  Starting from sector 16, we may find :
     *  - a CD-ROM Volume Descriptor Set (ISO9660) containing one or more Volume Descriptors
     *  - an Extended Area (UDF) -- [E] 2/8.3.1 and [U] 2.1.7
     *  There is no explicit end so read 16 sectors and then give up */
    for( i=16; i<16+16; i++)
    {
        if (!VOLUME_ReadCDBlock(handle, buff, i*BLOCK_SIZE))
            continue;

        /* We are supposed to check "BEA01", "NSR0x" and "TEA01" IDs + verify tag checksum
         *  but we assume the volume is well-formatted */
        if (!memcmp(&buff[1], "BEA01", 5)) return FS_UDF;
    }

    offs = VOLUME_FindCdRomDataBestVoldesc( handle );
    if (!offs) return FS_UNKNOWN;

    if (!VOLUME_ReadCDBlock(handle, buff, offs))
        return FS_ERROR;

    /* check for the iso9660 identifier */
    if (!memcmp(&buff[1], "CD001", 5)) return FS_ISO9660;
    return FS_UNKNOWN;
}


/**************************************************************************
 *                        UDF_Find_PVD
 * Find the Primary Volume Descriptor
 */
static BOOL UDF_Find_PVD( HANDLE handle, BYTE pvd[] )
{
    unsigned int i;
    DWORD offset;
    INT locations[] = { 256, -1, -257, 512 };

    for(i=0; i<ARRAY_SIZE(locations); i++)
    {
        if (!VOLUME_ReadCDBlock(handle, pvd, locations[i]*BLOCK_SIZE))
            return FALSE;

        /* Tag Identifier of Anchor Volume Descriptor Pointer is 2 -- [E] 3/10.2.1 */
        if (pvd[0]==2 && pvd[1]==0)
        {
            /* Tag location (Uint32) at offset 12, little-endian */
            offset  = pvd[20 + 0];
            offset |= pvd[20 + 1] << 8;
            offset |= pvd[20 + 2] << 16;
            offset |= pvd[20 + 3] << 24;
            offset *= BLOCK_SIZE;

            if (!VOLUME_ReadCDBlock(handle, pvd, offset))
                return FALSE;

            /* Check for the Primary Volume Descriptor Tag Id -- [E] 3/10.1.1 */
            if (pvd[0]!=1 || pvd[1]!=0)
                return FALSE;

            /* 8 or 16 bits per character -- [U] 2.1.1 */
            if (!(pvd[24]==8 || pvd[24]==16))
                return FALSE;

            return TRUE;
        }
    }

    return FALSE;
}


/**************************************************************************
 *                              VOLUME_GetSuperblockLabel
 */
static void VOLUME_GetSuperblockLabel( struct volume *volume, HANDLE handle, const BYTE *superblock )
{
    const BYTE *label_ptr = NULL;
    DWORD label_len;
    BYTE pvd[BLOCK_SIZE];

    switch (volume->fs_type)
    {
    case FS_ERROR:
        label_len = 0;
        break;
    case FS_UNKNOWN:
        get_filesystem_label( volume );
        return;
    case FS_FAT1216:
        label_ptr = superblock + 0x2b;
        label_len = 11;
        break;
    case FS_FAT32:
        label_ptr = superblock + 0x47;
        label_len = 11;
        break;
    case FS_ISO9660:
        {
            BYTE ver = superblock[0x5a];

            if (superblock[0x58] == 0x25 && superblock[0x59] == 0x2f &&  /* Unicode ID */
                ((ver == 0x40) || (ver == 0x43) || (ver == 0x45)))
            { /* yippee, unicode */
                unsigned int i;

                for (i = 0; i < 16; i++)
                    volume->label[i] = (superblock[40+2*i] << 8) | superblock[41+2*i];
                volume->label[i] = 0;
                while (i && volume->label[i-1] == ' ') volume->label[--i] = 0;
                return;
            }
            label_ptr = superblock + 40;
            label_len = 32;
            break;
        }
    case FS_UDF:
        if(!UDF_Find_PVD(handle, pvd))
        {
            label_len = 0;
            break;
        }

        /* [E] 3/10.1.4 and [U] 2.1.1 */
        if(pvd[24]==8)
        {
            label_ptr = pvd + 24 + 1;
            label_len = pvd[24+32-1];
            break;
        }
        else
        {
            unsigned int i;

            label_len = 1 + pvd[24+32-1];
            for (i = 0; i < label_len; i += 2)
                volume->label[i/2] = (pvd[24+1+i] << 8) | pvd[24+1+i+1];
            volume->label[label_len] = 0;
            return;
        }
    }
    if (label_len) RtlMultiByteToUnicodeN( volume->label, sizeof(volume->label) - sizeof(WCHAR),
                                           &label_len, (const char *)label_ptr, label_len );
    label_len /= sizeof(WCHAR);
    volume->label[label_len] = 0;
    while (label_len && volume->label[label_len-1] == ' ') volume->label[--label_len] = 0;
}


/**************************************************************************
 *                              UDF_Find_FSD_Sector
 * Find the File Set Descriptor used to compute the serial of a UDF volume
 */
static int UDF_Find_FSD_Sector( HANDLE handle, BYTE block[] )
{
    int i, PVD_sector, PD_sector, PD_length;

    if(!UDF_Find_PVD(handle,block))
        goto default_sector;

    /* Retrieve the tag location of the PVD -- [E] 3/7.2 */
    PVD_sector  = block[12 + 0];
    PVD_sector |= block[12 + 1] << 8;
    PVD_sector |= block[12 + 2] << 16;
    PVD_sector |= block[12 + 3] << 24;

    /* Find the Partition Descriptor */
    for(i=PVD_sector+1; ; i++)
    {
        if(!VOLUME_ReadCDBlock(handle, block, i*BLOCK_SIZE))
            goto default_sector;

        /* Partition Descriptor Tag Id -- [E] 3/10.5.1 */
        if(block[0]==5 && block[1]==0)
            break;

        /* Terminating Descriptor Tag Id -- [E] 3/10.9.1 */
        if(block[0]==8 && block[1]==0)
            goto default_sector;
    }

    /* Find the partition starting location -- [E] 3/10.5.8 */
    PD_sector  = block[188 + 0];
    PD_sector |= block[188 + 1] << 8;
    PD_sector |= block[188 + 2] << 16;
    PD_sector |= block[188 + 3] << 24;

    /* Find the partition length -- [E] 3/10.5.9 */
    PD_length  = block[192 + 0];
    PD_length |= block[192 + 1] << 8;
    PD_length |= block[192 + 2] << 16;
    PD_length |= block[192 + 3] << 24;

    for(i=PD_sector; i<PD_sector+PD_length; i++)
    {
        if(!VOLUME_ReadCDBlock(handle, block, i*BLOCK_SIZE))
            goto default_sector;

        /* File Set Descriptor Tag Id -- [E] 3/14.1.1 */
        if(block[0]==0 && block[1]==1)
            return i;
    }

default_sector:
    WARN("FSD sector not found, serial may be incorrect\n");
    return 257;
}


/**************************************************************************
 *                              VOLUME_GetSuperblockSerial
 */
static void VOLUME_GetSuperblockSerial( struct volume *volume, HANDLE handle, const BYTE *superblock )
{
    int FSD_sector;
    BYTE block[BLOCK_SIZE];

    switch (volume->fs_type)
    {
    case FS_ERROR:
        break;
    case FS_UNKNOWN:
        get_filesystem_serial( volume );
        break;
    case FS_FAT1216:
        volume->serial = GETLONG( superblock, 0x27 );
        break;
    case FS_FAT32:
        volume->serial = GETLONG( superblock, 0x43 );
        break;
    case FS_UDF:
        FSD_sector = UDF_Find_FSD_Sector(handle, block);
        if (!VOLUME_ReadCDBlock(handle, block, FSD_sector*BLOCK_SIZE))
            break;
        superblock = block;
        /* fallthrough */
    case FS_ISO9660:
        {
            BYTE sum[4];
            int i;

            sum[0] = sum[1] = sum[2] = sum[3] = 0;
            for (i = 0; i < 2048; i += 4)
            {
                /* DON'T optimize this into DWORD !! (breaks overflow) */
                sum[0] += superblock[i+0];
                sum[1] += superblock[i+1];
                sum[2] += superblock[i+2];
                sum[3] += superblock[i+3];
            }
            /*
             * OK, another braindead one... argh. Just believe it.
             * Me$$ysoft chose to reverse the serial number in NT4/W2K.
             * It's true and nobody will ever be able to change it.
             */
            if ((GetVersion() & 0x80000000) || volume->fs_type == FS_UDF)
                volume->serial = (sum[3] << 24) | (sum[2] << 16) | (sum[1] << 8) | sum[0];
            else
                volume->serial = (sum[0] << 24) | (sum[1] << 16) | (sum[2] << 8) | sum[3];
        }
    }
}


/**************************************************************************
 *                              VOLUME_GetAudioCDSerial
 */
static DWORD VOLUME_GetAudioCDSerial( const CDROM_TOC *toc )
{
    DWORD serial = 0;
    int i;

    for (i = 0; i <= toc->LastTrack - toc->FirstTrack; i++)
        serial += ((toc->TrackData[i].Address[1] << 16) |
                   (toc->TrackData[i].Address[2] << 8) |
                   toc->TrackData[i].Address[3]);

    /*
     * dwStart, dwEnd collect the beginning and end of the disc respectively, in
     * frames.
     * There it is collected for correcting the serial when there are less than
     * 3 tracks.
     */
    if (toc->LastTrack - toc->FirstTrack + 1 < 3)
    {
        DWORD dwStart = FRAME_OF_TOC(toc, toc->FirstTrack);
        DWORD dwEnd = FRAME_OF_TOC(toc, toc->LastTrack + 1);
        serial += dwEnd - dwStart;
    }
    return serial;
}


/* create the disk device for a given volume */
static NTSTATUS create_disk_device( enum device_type type, struct disk_device **device_ret, struct volume *volume )
{
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
        format = L"\\Device\\Harddisk%u";
        link_format = L"\\??\\PhysicalDrive%u";
        break;
    case DEVICE_HARDDISK_VOL:
        format = L"\\Device\\HarddiskVolume%u";
        first = 1;  /* harddisk volumes start counting from 1 */
        break;
    case DEVICE_FLOPPY:
        format = L"\\Device\\Floppy%u";
        break;
    case DEVICE_CDROM:
    case DEVICE_DVD:
        format = L"\\Device\\CdRom%u";
        link_format = L"\\??\\CdRom%u";
        break;
    case DEVICE_RAMDISK:
        format = L"\\Device\\Ramdisk%u";
        break;
    }

    name.MaximumLength = (lstrlenW(format) + 10) * sizeof(WCHAR);
    name.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, name.MaximumLength );
    for (i = first; i < 32; i++)
    {
        swprintf( name.Buffer, name.MaximumLength / sizeof(WCHAR), format, i );
        name.Length = lstrlenW(name.Buffer) * sizeof(WCHAR);
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
        device->volume         = volume;

        if (link_format)
        {
            UNICODE_STRING symlink;

            symlink.MaximumLength = (lstrlenW(link_format) + 10) * sizeof(WCHAR);
            if ((symlink.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, symlink.MaximumLength )))
            {
                swprintf( symlink.Buffer, symlink.MaximumLength / sizeof(WCHAR), link_format, i );
                symlink.Length = lstrlenW(symlink.Buffer) * sizeof(WCHAR);
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
        FIXME( "IoCreateDevice %s got %lx\n", debugstr_w(name.Buffer), status );
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
    free( device->unix_device );
    free( device->unix_mount );
    free( device->serial );
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
        free( volume );
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
        if ((volume->udi = strdup( udi ))) grab_volume( volume );
    }
    else if (volume->udi)
    {
        free( volume->udi );
        volume->udi = NULL;
        release_volume( volume );
    }
}

/* create a disk volume */
static NTSTATUS create_volume( const char *udi, enum device_type type, struct volume **volume_ret )
{
    struct volume *volume;
    NTSTATUS status;

    if (!(volume = calloc( 1, sizeof(*volume) )))
        return STATUS_NO_MEMORY;

    if (!(status = create_disk_device( type, &volume->device, volume )))
    {
        if (udi) set_volume_udi( volume, udi );
        list_add_tail( &volumes_list, &volume->entry );
        *volume_ret = grab_volume( volume );
    }
    else free( volume );

    return status;
}

/* create the disk device for a given volume */
static NTSTATUS create_dos_device( struct volume *volume, const char *udi, int letter,
                                   enum device_type type, struct dos_drive **drive_ret )
{
    struct dos_drive *drive;
    NTSTATUS status;

    if (!(drive = malloc( sizeof(*drive) ))) return STATUS_NO_MEMORY;
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
    else free( drive );

    return status;
}

/* delete the disk device for a given drive */
static void delete_dos_device( struct dos_drive *drive )
{
    list_remove( &drive->entry );
    if (drive->mount) delete_mount_point( drive->mount );
    release_volume( drive->volume );
    free( drive );
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

static BOOL get_volume_device_info( struct volume *volume )
{
    const char *unix_device = volume->device->unix_device;
    WCHAR *name;
    HANDLE handle;
    CDROM_TOC toc;
    DWORD size;
    BYTE superblock[SUPERBLOCK_SIZE];

    if (!unix_device)
        return FALSE;

    if (MOUNTMGR_CALL( check_device_access, volume->device->unix_device )) return FALSE;

    if (!(name = wine_get_dos_file_name( unix_device )))
    {
        ERR("Failed to convert %s to NT, err %lu\n", debugstr_a(unix_device), GetLastError());
        return FALSE;
    }
    handle = CreateFileW( name, GENERIC_READ | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, 0 );
    HeapFree( GetProcessHeap(), 0, name );
    if (handle == INVALID_HANDLE_VALUE)
    {
        WARN("Failed to open %s, err %lu\n", debugstr_a(unix_device), GetLastError());
        return FALSE;
    }

    if (DeviceIoControl( handle, IOCTL_CDROM_READ_TOC, NULL, 0, &toc, sizeof(toc), &size, 0 ))
    {
        if (!(toc.TrackData[0].Control & 0x04))  /* audio track */
        {
            TRACE( "%s: found audio CD\n", debugstr_a(unix_device) );
            wcscpy( volume->label, L"Audio CD" );
            volume->serial = VOLUME_GetAudioCDSerial( &toc );
            volume->fs_type = FS_ISO9660;
            CloseHandle( handle );
            return TRUE;
        }
        volume->fs_type = VOLUME_ReadCDSuperblock( handle, superblock );
    }
    else
    {
        if(GetLastError() == ERROR_NOT_READY)
        {
            TRACE( "%s: removable drive with no inserted media\n", debugstr_a(unix_device) );
            volume->fs_type = FS_UNKNOWN;
            CloseHandle( handle );
            return TRUE;
        }

        volume->fs_type = VOLUME_ReadFATSuperblock( handle, superblock );
        if (volume->fs_type == FS_UNKNOWN) volume->fs_type = VOLUME_ReadCDSuperblock( handle, superblock );
    }

    TRACE( "%s: found fs type %d\n", debugstr_a(unix_device), volume->fs_type );
    if (volume->fs_type == FS_ERROR)
    {
        CloseHandle( handle );
        return FALSE;
    }

    VOLUME_GetSuperblockLabel( volume, handle, superblock );
    VOLUME_GetSuperblockSerial( volume, handle, superblock );

    CloseHandle( handle );
    return TRUE;
}

/* set disk serial for dos devices that reside on a given Unix device */
static void set_dos_devices_disk_serial( struct disk_device *device )
{
    unsigned int devices;
    struct dos_drive *drive;
    struct get_volume_dos_devices_params params = { device->unix_mount, &devices };

    if (!device->serial || !device->unix_mount || MOUNTMGR_CALL( get_volume_dos_devices, &params ))
        return;

    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        /* drives mapped to Unix devices already have serial set, if available */
        if (drive->volume->device->unix_device) continue;
        /* copy serial if drive resides on this Unix device */
        if (devices & (1 << drive->drive))
        {
            free( drive->volume->device->serial );
            drive->volume->device->serial = strdup( device->serial );
        }
    }
}

/* change the information for an existing volume */
static NTSTATUS set_volume_info( struct volume *volume, struct dos_drive *drive, const char *device,
                                 const char *mount_point, enum device_type type, const GUID *guid,
                                 const char *disk_serial )
{
    void *id = NULL;
    unsigned int id_len = 0;
    struct disk_device *disk_device = volume->device;
    NTSTATUS status;

    if (type != disk_device->type)
    {
        if ((status = create_disk_device( type, &disk_device, volume ))) return status;
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
        free( disk_device->unix_device );
        free( disk_device->unix_mount );
        free( disk_device->serial );
    }
    disk_device->unix_device = strdup( device );
    disk_device->unix_mount = strdup( mount_point );
    disk_device->serial = strdup( disk_serial );
    set_dos_devices_disk_serial( disk_device );

    if (!get_volume_device_info( volume ))
    {
        if (volume->device->type == DEVICE_CDROM)
            volume->fs_type = FS_ISO9660;
        else if (volume->device->type == DEVICE_DVD)
            volume->fs_type = FS_UDF;
        else
            volume->fs_type = FS_UNKNOWN;

        get_filesystem_label( volume );
        get_filesystem_serial( volume );
    }

    TRACE("fs_type %#x, label %s, serial %08lx\n", volume->fs_type, debugstr_w(volume->label), volume->serial);

    if (guid && memcmp( &volume->guid, guid, sizeof(volume->guid) ))
    {
        volume->guid = *guid;
        if (volume->mount)
        {
            delete_mount_point( volume->mount );
            volume->mount = NULL;
        }
    }

    if (!volume->serial)
        memcpy(&volume->serial, &volume->guid.Data4[4], sizeof(DWORD));

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

/* create devices for mapped drives */
static void create_drive_devices(void)
{
    char dosdev[] = "a::";
    struct dos_drive *drive;
    struct volume *volume;
    unsigned int i;
    HKEY drives_key;
    enum device_type drive_type;
    WCHAR driveW[] = L"a:";

    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, L"Software\\Wine\\Drives", &drives_key )) drives_key = 0;

    for (i = 0; i < MAX_DOS_DRIVES; i++)
    {
        char link[4096], unix_dev[4096];
        char *device = NULL;
        struct get_dosdev_symlink_params params = { dosdev, link, sizeof(link) };

        dosdev[0] = 'a' + i;
        dosdev[2] = 0;
        if (MOUNTMGR_CALL( get_dosdev_symlink, &params )) continue;
        dosdev[2] = ':';
        params.dest = unix_dev;
        params.size = sizeof(unix_dev);
        if (!MOUNTMGR_CALL( get_dosdev_symlink, &params )) device = unix_dev;

        drive_type = i < 2 ? DEVICE_FLOPPY : DEVICE_HARDDISK_VOL;
        if (drives_key)
        {
            WCHAR buffer[32];
            DWORD j, type, size = sizeof(buffer);

            driveW[0] = 'a' + i;
            if (!RegQueryValueExW( drives_key, driveW, NULL, &type, (BYTE *)buffer, &size ) &&
                type == REG_SZ)
            {
                for (j = 0; j < ARRAY_SIZE(drive_types); j++)
                    if (drive_types[j][0] && !wcsicmp( buffer, drive_types[j] ))
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
            set_volume_info( drive->volume, drive, device, link, drive_type, guid, NULL );
        }
        if (volume) release_volume( volume );
    }
    RegCloseKey( drives_key );
}

/* fill in the "Logical Unit" key for a given SCSI address */
static void create_scsi_entry( struct volume *volume, const struct scsi_info *info )
{
    static UCHAR tape_no = 0;

    WCHAR dataW[50];
    DWORD sizeW;
    DWORD value;
    const char *data;
    HKEY scsi_key;
    HKEY port_key;
    HKEY bus_key;
    HKEY target_key;
    HKEY lun_key;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\Scsi", 0, KEY_READ|KEY_WRITE, &scsi_key )) return;

    swprintf( dataW, ARRAY_SIZE( dataW ), L"Scsi Port %d", info->addr.PortNumber );
    if (RegCreateKeyExW( scsi_key, dataW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &port_key, NULL )) return;
    RegCloseKey( scsi_key );

    RtlMultiByteToUnicodeN( dataW, sizeof(dataW), &sizeW, info->driver, strlen(info->driver)+1);
    RegSetValueExW( port_key, L"Driver", 0, REG_SZ, (const BYTE *)dataW, sizeW );
    value = 10;
    RegSetValueExW( port_key, L"FirstBusTimeScanInMs", 0, REG_DWORD, (const BYTE *)&value, sizeof(value));

    value = 0;

    swprintf( dataW, ARRAY_SIZE( dataW ), L"Scsi Bus %d", info->addr.PathId );
    if (RegCreateKeyExW( port_key, dataW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &bus_key, NULL )) return;
    RegCloseKey( port_key );

    swprintf( dataW, ARRAY_SIZE( dataW ), L"Initiator Id %d", info->init_id );
    if (RegCreateKeyExW( bus_key, dataW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &target_key, NULL )) return;
    RegCloseKey( target_key );

    swprintf( dataW, ARRAY_SIZE( dataW ), L"Target Id %d", info->addr.TargetId );
    if (RegCreateKeyExW( bus_key, dataW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &target_key, NULL )) return;
    RegCloseKey( bus_key );

    swprintf( dataW, ARRAY_SIZE( dataW ), L"Logical Unit Id %d", info->addr.Lun );
    if (RegCreateKeyExW( target_key, dataW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &lun_key, NULL )) return;
    RegCloseKey( target_key );

    switch (info->type)
    {
    case SCSI_DISK_PERIPHERAL:              data = "DiskPeripheral"; break;
    case SCSI_TAPE_PERIPHERAL:              data = "TapePeripheral"; break;
    case SCSI_PRINTER_PERIPHERAL:           data = "PrinterPeripheral"; break;
    case SCSI_WORM_PERIPHERAL:              data = "WormPeripheral"; break;
    case SCSI_CDROM_PERIPHERAL:             data = "CdRomPeripheral"; break;
    case SCSI_SCANNER_PERIPHERAL:           data = "ScannerPeripheral"; break;
    case SCSI_OPTICAL_DISK_PERIPHERAL:      data = "OpticalDiskPeripheral"; break;
    case SCSI_MEDIUM_CHANGER_PERIPHERAL:    data = "MediumChangerPeripheral"; break;
    case SCSI_COMMS_PERIPHERAL:             data = "CommunicationsPeripheral"; break;
    case SCSI_ASC_GRAPHICS_PERIPHERAL:
    case SCSI_ASC_GRAPHICS2_PERIPHERAL:     data = "ASCPrePressGraphicsPeripheral"; break;
    case SCSI_ARRAY_PERIPHERAL:             data = "ArrayPeripheral"; break;
    case SCSI_ENCLOSURE_PERIPHERAL:         data = "EnclosurePeripheral"; break;
    case SCSI_REDUCED_DISK_PERIPHERAL:      data = "RBCPeripheral"; break;
    case SCSI_CARD_READER_PERIPHERAL:       data = "CardReaderPeripheral"; break;
    case SCSI_BRIDGE_PERIPHERAL:            data = "BridgePeripheral"; break;
    case SCSI_OBJECT_STORAGE_PERIPHERAL:    /* Object-based storage devices */
    case SCSI_DRIVE_CONTROLLER_PERIPHERAL:  /* Automation/drive controllers */
    case SCSI_REDUCED_CDROM_PERIPHERAL:     /* Reduced-commands MM devices */
    case SCSI_PROCESSOR_PERIPHERAL:         /* Processor devices (considered to be "Other" by Windows) */
    default:                                data = "OtherPeripheral"; break;
    }
    RtlMultiByteToUnicodeN( dataW, sizeof(dataW), &sizeW, data, strlen(data)+1);
    RegSetValueExW( lun_key, L"Type", 0, REG_SZ, (const BYTE *)dataW, sizeW );

    RtlMultiByteToUnicodeN( dataW, sizeof(dataW), &sizeW, info->model, strlen(info->model)+1);
    RegSetValueExW( lun_key, L"Identifier", 0, REG_SZ, (const BYTE *)dataW, sizeW );

    if (volume)
    {
        UNICODE_STRING *dev = &volume->device->name;
        WCHAR *buffer = wcschr( dev->Buffer+1, '\\' ) + 1;
        ULONG length = dev->Length - (buffer - dev->Buffer)*sizeof(WCHAR);
        RegSetValueExW( lun_key, L"DeviceName", 0, REG_SZ, (const BYTE *)buffer, length );
    }
    else if (info->type == SCSI_TAPE_PERIPHERAL)
    {
        swprintf( dataW, ARRAY_SIZE( dataW ), L"Tape%d", tape_no++ );
        RegSetValueExW( lun_key, L"DeviceName", 0, REG_SZ, (const BYTE *)dataW, lstrlenW( dataW ) );
    }

    RegCloseKey( lun_key );
}

/* create a new disk volume */
NTSTATUS add_volume( const char *udi, const char *device, const char *mount_point,
                     enum device_type type, const GUID *guid, const char *disk_serial,
                     const struct scsi_info *scsi_info )
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
    if (!status) status = set_volume_info( volume, NULL, device, mount_point, type, guid, disk_serial );
    if (!status && scsi_info) create_scsi_entry( volume, scsi_info );
    if (volume) release_volume( volume );
    LeaveCriticalSection( &device_section );
    return status;
}

/* remove a disk volume */
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
                         const char *mount_point, enum device_type type, const GUID *guid,
                         const struct scsi_info *scsi_info )
{
    HKEY hkey;
    NTSTATUS status = STATUS_SUCCESS;
    struct dos_drive *drive, *next;
    struct volume *volume;
    int notify = -1;
    char dosdev[] = "a::";

    EnterCriticalSection( &device_section );
    volume = find_matching_volume( udi, device, mount_point, type );

    if (letter == -1)  /* auto-assign a letter */
    {
        struct add_drive_params params = { device, type, &letter };
        if ((status = MOUNTMGR_CALL( add_drive, &params ))) goto done;

        LIST_FOR_EACH_ENTRY_SAFE( drive, next, &drives_list, struct dos_drive, entry )
        {
            if (drive->volume->udi && !strcmp( udi, drive->volume->udi )) goto found;
            if (drive->drive == letter) delete_dos_device( drive );
        }
    }
    else  /* simply reset the device symlink */
    {
        struct set_dosdev_symlink_params params = { dosdev, device };

        LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
            if (drive->drive == letter) break;

        dosdev[0] = 'a' + letter;
        if (&drive->entry == &drives_list)
        {
            MOUNTMGR_CALL( set_dosdev_symlink, &params );
        }
        else
        {
            if (!device || !drive->volume->device->unix_device ||
                strcmp( device, drive->volume->device->unix_device ))
                MOUNTMGR_CALL( set_dosdev_symlink, &params );
            delete_dos_device( drive );
        }
    }

    if ((status = create_dos_device( volume, udi, letter, type, &drive ))) goto done;

found:
    if (!guid && !volume) guid = get_default_uuid( letter );
    if (!volume) volume = grab_volume( drive->volume );
    set_drive_info( drive, letter, volume );
    dosdev[0] = 'a' + drive->drive;
    dosdev[2] = 0;
    if (!mount_point || !volume->device->unix_mount || strcmp( mount_point, volume->device->unix_mount ))
    {
        struct set_dosdev_symlink_params params = { dosdev, mount_point };
        MOUNTMGR_CALL( set_dosdev_symlink, &params );
    }
    set_volume_info( volume, drive, device, mount_point, type, guid, NULL );

    TRACE( "added device %c: udi %s for %s on %s type %u\n",
           'a' + drive->drive, wine_dbgstr_a(udi), wine_dbgstr_a(device),
           wine_dbgstr_a(mount_point), type );

    /* hack: force the drive type in the registry */
    if (!RegCreateKeyW( HKEY_LOCAL_MACHINE, L"Software\\Wine\\Drives", &hkey ))
    {
        const WCHAR *type_name = drive_types[type];
        WCHAR name[] = L"a:";

        name[0] += drive->drive;
        if (!type_name[0] && type == DEVICE_HARDDISK) type_name = drive_types[DEVICE_FLOPPY];
        if (type_name[0])
            RegSetValueExW( hkey, name, 0, REG_SZ, (const BYTE *)type_name,
                            (lstrlenW(type_name) + 1) * sizeof(WCHAR) );
        else
            RegDeleteValueW( hkey, name );
        RegCloseKey( hkey );
    }

    if (udi) notify = drive->drive;
    if (scsi_info) create_scsi_entry( volume, scsi_info );

done:
    if (volume) release_volume( volume );
    LeaveCriticalSection( &device_section );
    if (notify != -1) send_notify( notify, DBT_DEVICEARRIVAL );
    return status;
}

/* remove an existing dos drive, by letter or udi */
NTSTATUS remove_dos_device( int letter, const char *udi )
{
    NTSTATUS status = STATUS_NO_SUCH_DEVICE;
    HKEY hkey;
    struct dos_drive *drive;
    char dosdev[] = "a:";
    int notify = -1;

    EnterCriticalSection( &device_section );
    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        struct set_dosdev_symlink_params params = { dosdev, NULL };

        if (udi)
        {
            if (!drive->volume->udi) continue;
            if (strcmp( udi, drive->volume->udi )) continue;
            set_volume_udi( drive->volume, NULL );
        }
        else if (drive->drive != letter) continue;

        dosdev[0] = 'a' + drive->drive;
        MOUNTMGR_CALL( set_dosdev_symlink, &params );

        /* clear the registry key too */
        if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, L"Software\\Wine\\Drives", &hkey ))
        {
            WCHAR name[] = L"a:";
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

static enum mountmgr_fs_type get_mountmgr_fs_type(enum fs_type fs_type)
{
    switch (fs_type)
    {
    case FS_ISO9660: return MOUNTMGR_FS_TYPE_ISO9660;
    case FS_UDF:     return MOUNTMGR_FS_TYPE_UDF;
    case FS_FAT1216: return MOUNTMGR_FS_TYPE_FAT;
    case FS_FAT32:   return MOUNTMGR_FS_TYPE_FAT32;
    default:         return MOUNTMGR_FS_TYPE_NTFS;
    }
}

/* query information about an existing dos drive, by letter or udi */
static struct volume *find_volume_by_letter( int letter )
{
    struct volume *volume = NULL;
    struct dos_drive *drive;

    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        if (drive->drive != letter) continue;
        volume = grab_volume( drive->volume );
        TRACE( "found matching volume %s for drive letter %c:\n", debugstr_guid(&volume->guid),
               'a' + letter );
        break;
    }
    return volume;
}

/* query information about an existing unix device, by dev_t */
static struct volume *find_volume_by_unixdev( ULONGLONG unix_dev )
{
    struct volume *volume;

    LIST_FOR_EACH_ENTRY( volume, &volumes_list, struct volume, entry )
    {
        struct match_unixdev_params params = { volume->device->unix_device, unix_dev };
        if (!volume->device->unix_device || !MOUNTMGR_CALL( match_unixdev, &params ))
            continue;

        TRACE( "found matching volume %s\n", debugstr_guid(&volume->guid) );
        return grab_volume( volume );
    }
    return NULL;
}

/* implementation of IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE */
NTSTATUS query_unix_drive( void *buff, SIZE_T insize, SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    const struct mountmgr_unix_drive *input = buff;
    struct mountmgr_unix_drive *output = NULL;
    char *device, *mount_point;
    int letter = towlower( input->letter );
    DWORD size, type = DEVICE_UNKNOWN, serial;
    NTSTATUS status = STATUS_SUCCESS;
    enum mountmgr_fs_type fs_type;
    enum device_type device_type;
    struct volume *volume;
    char *ptr;
    WCHAR *label;

    if (letter && (letter < 'a' || letter > 'z')) return STATUS_INVALID_PARAMETER;

    EnterCriticalSection( &device_section );
    if (letter)
        volume = find_volume_by_letter( letter - 'a' );
    else
        volume = find_volume_by_unixdev( input->unix_dev );
    if (volume)
    {
        device_type = volume->device->type;
        fs_type = get_mountmgr_fs_type( volume->fs_type );
        serial = volume->serial;
        device = strdup( volume->device->unix_device );
        mount_point = strdup( volume->device->unix_mount );
        label = wcsdup( volume->label );
        release_volume( volume );
    }
    LeaveCriticalSection( &device_section );

    if (!volume)
        return STATUS_NO_SUCH_DEVICE;

    switch (device_type)
    {
    case DEVICE_UNKNOWN:      type = DRIVE_UNKNOWN; break;
    case DEVICE_HARDDISK:     type = DRIVE_REMOVABLE; break;
    case DEVICE_HARDDISK_VOL: type = DRIVE_FIXED; break;
    case DEVICE_FLOPPY:       type = DRIVE_REMOVABLE; break;
    case DEVICE_CDROM:        type = DRIVE_CDROM; break;
    case DEVICE_DVD:          type = DRIVE_CDROM; break;
    case DEVICE_NETWORK:      type = DRIVE_REMOTE; break;
    case DEVICE_RAMDISK:      type = DRIVE_RAMDISK; break;
    }

    size = sizeof(*output);
    if (label) size += (lstrlenW(label) + 1) * sizeof(WCHAR);
    if (device) size += strlen(device) + 1;
    if (mount_point) size += strlen(mount_point) + 1;

    input = NULL;
    output = buff;
    output->size = size;
    output->letter = letter;
    output->type = type;
    output->fs_type = fs_type;
    output->serial = serial;
    output->mount_point_offset = 0;
    output->device_offset = 0;
    output->label_offset = 0;

    ptr = (char *)(output + 1);

    if (label && ptr + (lstrlenW(label) + 1) * sizeof(WCHAR) - (char *)output <= outsize)
    {
        output->label_offset = ptr - (char *)output;
        lstrcpyW( (WCHAR *)ptr, label );
        ptr += (lstrlenW(label) + 1) * sizeof(WCHAR);
    }
    if (mount_point && ptr + strlen(mount_point) + 1 - (char *)output <= outsize)
    {
        output->mount_point_offset = ptr - (char *)output;
        strcpy( ptr, mount_point );
        ptr += strlen(ptr) + 1;
    }
    if (device && ptr + strlen(device) + 1 - (char *)output <= outsize)
    {
        output->device_offset = ptr - (char *)output;
        strcpy( ptr, device );
        ptr += strlen(ptr) + 1;
    }

    TRACE( "returning %c: dev %s mount %s type %lu\n",
           letter, debugstr_a(device), debugstr_a(mount_point), type );

    iosb->Information = ptr - (char *)output;
    if (size > outsize) status = STATUS_BUFFER_OVERFLOW;

    free( device );
    free( mount_point );
    free( label );
    return status;
}

static NTSTATUS query_property( struct disk_device *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    STORAGE_PROPERTY_QUERY *query = irp->AssociatedIrp.SystemBuffer;
    NTSTATUS status;

    if (!irp->AssociatedIrp.SystemBuffer
        || irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_PROPERTY_QUERY))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Try to persuade application not to check property */
    if (query->QueryType == PropertyExistsQuery)
    {
        return STATUS_NOT_SUPPORTED;
    }

    switch (query->PropertyId)
    {
    case StorageDeviceProperty:
    {
        STORAGE_DEVICE_DESCRIPTOR *descriptor;
        DWORD len = sizeof(*descriptor);

        if (device->serial) len += strlen( device->serial ) + 1;

        if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_DESCRIPTOR_HEADER))
            status = STATUS_INVALID_PARAMETER;
        else if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < len)
        {
            descriptor = irp->AssociatedIrp.SystemBuffer;
            descriptor->Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
            descriptor->Size = len;
            irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            status = STATUS_SUCCESS;
        }
        else
        {
            FIXME( "Faking StorageDeviceProperty data\n" );

            memset( irp->AssociatedIrp.SystemBuffer, 0, irpsp->Parameters.DeviceIoControl.OutputBufferLength );
            descriptor = irp->AssociatedIrp.SystemBuffer;
            descriptor->Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
            descriptor->Size = len;
            descriptor->DeviceType = FILE_DEVICE_DISK;
            descriptor->DeviceTypeModifier = 0;
            descriptor->RemovableMedia = FALSE;
            descriptor->CommandQueueing = FALSE;
            descriptor->VendorIdOffset = 0;
            descriptor->ProductIdOffset = 0;
            descriptor->ProductRevisionOffset = 0;
            descriptor->BusType = BusTypeScsi;
            descriptor->RawPropertiesLength = 0;
            if (!device->serial) descriptor->SerialNumberOffset = 0;
            else
            {
                descriptor->SerialNumberOffset = sizeof(*descriptor);
                strcpy( (char *)descriptor + descriptor->SerialNumberOffset, device->serial );
            }
            irp->IoStatus.Information = len;
            status = STATUS_SUCCESS;
        }

        break;
    }
    default:
        FIXME( "Unsupported property %#x\n", query->PropertyId );
        status = STATUS_NOT_SUPPORTED;
        break;
    }
    return status;
}

static NTSTATUS WINAPI harddisk_query_volume( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    int info_class = irpsp->Parameters.QueryVolume.FsInformationClass;
    ULONG length = irpsp->Parameters.QueryVolume.Length;
    struct disk_device *dev = device->DeviceExtension;
    PIO_STATUS_BLOCK io = &irp->IoStatus;
    struct volume *volume;
    NTSTATUS status;

    TRACE( "volume query %x length %lu\n", info_class, length );

    EnterCriticalSection( &device_section );
    volume = dev->volume;
    if (!volume)
    {
        status = STATUS_BAD_DEVICE_TYPE;
        goto done;
    }

    switch(info_class)
    {
    case FileFsVolumeInformation:
    {

        FILE_FS_VOLUME_INFORMATION *info = irp->AssociatedIrp.SystemBuffer;

        if (length < sizeof(FILE_FS_VOLUME_INFORMATION))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        info->VolumeCreationTime.QuadPart = 0; /* FIXME */
        info->VolumeSerialNumber = volume->serial;
        info->VolumeLabelLength = min( lstrlenW(volume->label) * sizeof(WCHAR),
                                       length - offsetof( FILE_FS_VOLUME_INFORMATION, VolumeLabel ) );
        info->SupportsObjects = (get_mountmgr_fs_type(volume->fs_type) == MOUNTMGR_FS_TYPE_NTFS);
        memcpy( info->VolumeLabel, volume->label, info->VolumeLabelLength );

        io->Information = offsetof( FILE_FS_VOLUME_INFORMATION, VolumeLabel ) + info->VolumeLabelLength;
        status = STATUS_SUCCESS;
        break;
    }
    case FileFsSizeInformation:
    {
        FILE_FS_SIZE_INFORMATION *info = irp->AssociatedIrp.SystemBuffer;
        struct size_info size_info = { 0, 0, 0, 0, 0 };
        struct get_volume_size_info_params params = { dev->unix_mount, &size_info };

        if (length < sizeof(FILE_FS_SIZE_INFORMATION))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if ((status = MOUNTMGR_CALL( get_volume_size_info, &params )) == STATUS_SUCCESS)
        {
            info->TotalAllocationUnits.QuadPart = size_info.total_allocation_units;
            info->AvailableAllocationUnits.QuadPart = size_info.caller_available_allocation_units;
            info->SectorsPerAllocationUnit = size_info.sectors_per_allocation_unit;
            info->BytesPerSector = size_info.bytes_per_sector;
            io->Information = sizeof(*info);
            status = STATUS_SUCCESS;
        }

        break;
    }
    case FileFsAttributeInformation:
    {
        FILE_FS_ATTRIBUTE_INFORMATION *info = irp->AssociatedIrp.SystemBuffer;
        enum mountmgr_fs_type fs_type = get_mountmgr_fs_type(volume->fs_type);
        const WCHAR *fsname;

        if (length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        switch (fs_type)
        {
        case MOUNTMGR_FS_TYPE_ISO9660:
            fsname = L"CDFS";
            info->FileSystemAttributes = FILE_READ_ONLY_VOLUME;
            info->MaximumComponentNameLength = 221;
            break;
        case MOUNTMGR_FS_TYPE_UDF:
            fsname = L"UDF";
            info->FileSystemAttributes = FILE_READ_ONLY_VOLUME | FILE_UNICODE_ON_DISK | FILE_CASE_SENSITIVE_SEARCH;
            info->MaximumComponentNameLength = 255;
            break;
        case MOUNTMGR_FS_TYPE_FAT:
            fsname = L"FAT";
            info->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES; /* FIXME */
            info->MaximumComponentNameLength = 255;
            break;
        case MOUNTMGR_FS_TYPE_FAT32:
            fsname = L"FAT32";
            info->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES; /* FIXME */
            info->MaximumComponentNameLength = 255;
            break;
        default:
            fsname = L"NTFS";
            info->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES | FILE_PERSISTENT_ACLS;
            info->MaximumComponentNameLength = 255;
            break;
        }
        info->FileSystemNameLength = min( wcslen(fsname) * sizeof(WCHAR), length - offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) );
        memcpy(info->FileSystemName, fsname, info->FileSystemNameLength);
        io->Information = offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) + info->FileSystemNameLength;
        status = STATUS_SUCCESS;
        break;
    }
    case FileFsFullSizeInformation:
    {
        FILE_FS_FULL_SIZE_INFORMATION *info = irp->AssociatedIrp.SystemBuffer;
        struct size_info size_info = { 0, 0, 0, 0, 0 };
        struct get_volume_size_info_params params = { dev->unix_mount, &size_info };

        if (length < sizeof(FILE_FS_FULL_SIZE_INFORMATION))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if ((status = MOUNTMGR_CALL( get_volume_size_info, &params )) == STATUS_SUCCESS)
        {
            info->TotalAllocationUnits.QuadPart = size_info.total_allocation_units;
            info->CallerAvailableAllocationUnits.QuadPart = size_info.caller_available_allocation_units;
            info->ActualAvailableAllocationUnits.QuadPart = size_info.actual_available_allocation_units;
            info->SectorsPerAllocationUnit = size_info.sectors_per_allocation_unit;
            info->BytesPerSector = size_info.bytes_per_sector;
            io->Information = sizeof(*info);
            status = STATUS_SUCCESS;
        }

        break;
    }

    default:
        FIXME("Unsupported volume query %x\n", irpsp->Parameters.QueryVolume.FsInformationClass);
        status = STATUS_NOT_SUPPORTED;
        break;
    }

done:
    io->Status = status;
    LeaveCriticalSection( &device_section );
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

/* handler for ioctls on the harddisk device */
static NTSTATUS WINAPI harddisk_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    struct disk_device *dev = device->DeviceExtension;
    NTSTATUS status;

    TRACE( "ioctl %lx insize %lu outsize %lu\n",
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
        status = STATUS_SUCCESS;
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
        status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
    {
        DWORD len = min( sizeof(dev->devnum), irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        memcpy( irp->AssociatedIrp.SystemBuffer, &dev->devnum, len );
        irp->IoStatus.Information = len;
        status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_CDROM_READ_TOC:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
    {
        DWORD len = min( 32, irpsp->Parameters.DeviceIoControl.OutputBufferLength );

        FIXME( "returning zero-filled buffer for IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS\n" );
        memset( irp->AssociatedIrp.SystemBuffer, 0, len );
        irp->IoStatus.Information = len;
        status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_STORAGE_QUERY_PROPERTY:
        status = query_property( dev, irp );
        break;
    default:
    {
        ULONG code = irpsp->Parameters.DeviceIoControl.IoControlCode;
        FIXME("Unsupported ioctl %lx (device=%lx access=%lx func=%lx method=%lx)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
        status = STATUS_NOT_SUPPORTED;
        break;
    }
    }

    irp->IoStatus.Status = status;
    LeaveCriticalSection( &device_section );
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

/* driver entry point for the harddisk driver */
NTSTATUS WINAPI harddisk_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    struct disk_device *device;

    harddisk_driver = driver;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = harddisk_ioctl;
    driver->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = harddisk_query_volume;

    /* create a harddisk0 device that isn't assigned to any drive */
    create_disk_device( DEVICE_HARDDISK, &device, NULL );

    create_drive_devices();

    return STATUS_SUCCESS;
}


/* create a serial or parallel port */
static BOOL create_port_device( DRIVER_OBJECT *driver, int n, const char *unix_path,
                                const char *dosdevices_path, HKEY windows_ports_key )
{
    const WCHAR *dos_name_format, *nt_name_format, *reg_value_format, *symlink_format, *default_device;
    WCHAR dos_name[7], reg_value[256], nt_buffer[32], symlink_buffer[32];
    UNICODE_STRING nt_name, symlink_name, default_name;
    DEVICE_OBJECT *dev_obj;
    NTSTATUS status;
    struct set_dosdev_symlink_params params = { dosdevices_path, unix_path };

    /* create DOS device */
    if (MOUNTMGR_CALL( set_dosdev_symlink, &params )) return FALSE;

    if (driver == serial_driver)
    {
        dos_name_format = L"COM%u";
        nt_name_format = L"\\Device\\Serial%u";
        reg_value_format = L"COM%u";
        symlink_format = L"\\DosDevices\\COM%u";
        default_device = L"\\DosDevices\\AUX";
    }
    else
    {
        dos_name_format = L"LPT%u";
        nt_name_format = L"\\Device\\Parallel%u";
        reg_value_format = L"\\DosDevices\\LPT%u";
        symlink_format = L"\\DosDevices\\LPT%u";
        default_device = L"\\DosDevices\\PRN";
    }

    swprintf( dos_name, ARRAY_SIZE(dos_name), dos_name_format, n );

    /* create NT device */
    swprintf( nt_buffer, ARRAY_SIZE(nt_buffer), nt_name_format, n - 1 );
    RtlInitUnicodeString( &nt_name, nt_buffer );
    status = IoCreateDevice( driver, 0, &nt_name, 0, 0, FALSE, &dev_obj );
    if (status != STATUS_SUCCESS)
    {
        FIXME( "IoCreateDevice %s got %lx\n", debugstr_w(nt_name.Buffer), status );
        return FALSE;
    }
    swprintf( symlink_buffer, ARRAY_SIZE(symlink_buffer), symlink_format, n );
    RtlInitUnicodeString( &symlink_name, symlink_buffer );
    IoCreateSymbolicLink( &symlink_name, &nt_name );
    if (n == 1)
    {
        RtlInitUnicodeString( &default_name, default_device );
        IoCreateSymbolicLink( &default_name, &symlink_name );
    }

    /* TODO: store information about the Unix device in the NT device */

    /* create registry entry */
    swprintf( reg_value, ARRAY_SIZE(reg_value), reg_value_format, n );
    RegSetValueExW( windows_ports_key, nt_name.Buffer, 0, REG_SZ,
                    (BYTE *)reg_value, (lstrlenW( reg_value ) + 1) * sizeof(WCHAR) );

    return TRUE;
}

/* find and create serial or parallel ports */
static void create_port_devices( DRIVER_OBJECT *driver, const char *devices )
{
    const WCHAR *windows_ports_key_name;
    const char *dosdev_fmt;
    char dosdev[8];
    HKEY wine_ports_key = NULL, windows_ports_key = NULL;
    char unix_path[256];
    const WCHAR *port_prefix;
    WCHAR reg_value[256];
    BOOL used[MAX_PORTS];
    WCHAR port[7];
    DWORD port_len, type, size;
    int i, n;

    if (driver == serial_driver)
    {
        dosdev_fmt = "com%u";
        windows_ports_key_name = L"HARDWARE\\DEVICEMAP\\SERIALCOMM";
        port_prefix = L"COM";
    }
    else
    {
        dosdev_fmt = "lpt%u";
        windows_ports_key_name = L"HARDWARE\\DEVICEMAP\\PARALLEL PORTS";
        port_prefix = L"LPT";
    }

    /* @@ Wine registry key: HKLM\Software\Wine\Ports */

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\Wine\\Ports", 0, NULL, 0,
                     KEY_QUERY_VALUE, NULL, &wine_ports_key, NULL );
    RegCreateKeyExW( HKEY_LOCAL_MACHINE, windows_ports_key_name, 0, NULL, REG_OPTION_VOLATILE,
                     KEY_ALL_ACCESS, NULL, &windows_ports_key, NULL );

    /* add user-defined serial ports */
    memset(used, 0, sizeof(used));
    for (i = 0; ; i++)
    {
        port_len = ARRAY_SIZE(port);
        size = sizeof(reg_value);
        if (RegEnumValueW( wine_ports_key, i, port, &port_len, NULL,
                    &type, (BYTE*)reg_value, &size ) != ERROR_SUCCESS)
            break;
        if (type != REG_SZ || wcsnicmp( port, port_prefix, 3 ))
            continue;

        n = wcstol( port + 3, NULL, 10 );
        if (n < 1 || n >= MAX_PORTS)
            continue;

        if (!WideCharToMultiByte( CP_UNIXCP, WC_ERR_INVALID_CHARS, reg_value, size/sizeof(WCHAR),
                    unix_path, sizeof(unix_path), NULL, NULL))
            continue;

        used[n - 1] = TRUE;
        sprintf( dosdev, dosdev_fmt, n );
        create_port_device( driver, n, unix_path, dosdev, windows_ports_key );
    }

    /* look for ports in the usual places */

    for (n = 1; *devices; n++, devices += strlen(devices) + 1)
    {
        while (n <= MAX_PORTS && used[n - 1]) n++;
        if (n > MAX_PORTS) break;
        sprintf( dosdev, dosdev_fmt, n );
        create_port_device( driver, n, devices, dosdev, windows_ports_key );
    }

    RegCloseKey( wine_ports_key );
    RegCloseKey( windows_ports_key );
}

/* driver entry point for the serial port driver */
NTSTATUS WINAPI serial_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    char devices[4096];
    struct detect_ports_params params = { devices, sizeof(devices) };

    serial_driver = driver;
    /* TODO: fill in driver->MajorFunction */

    MOUNTMGR_CALL( detect_serial_ports, &params );
    create_port_devices( driver, devices );

    return STATUS_SUCCESS;
}

/* driver entry point for the parallel port driver */
NTSTATUS WINAPI parallel_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    char devices[4096];
    struct detect_ports_params params = { devices, sizeof(devices) };

    parallel_driver = driver;
    /* TODO: fill in driver->MajorFunction */

    MOUNTMGR_CALL( detect_parallel_ports, &params );
    create_port_devices( driver, devices );

    return STATUS_SUCCESS;
}
