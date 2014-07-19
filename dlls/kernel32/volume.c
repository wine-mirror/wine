/*
 * Volume management functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 1999 Petr Tomasek
 * Copyright 2000 Andreas Mohr
 * Copyright 2003 Eric Pouech
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winioctl.h"
#include "ntddcdrm.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"
#include "kernel_private.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(volume);

#define BLOCK_SIZE 2048
#define SUPERBLOCK_SIZE BLOCK_SIZE
#define SYMBOLIC_LINK_QUERY 0x0001

#define CDFRAMES_PERSEC         75
#define CDFRAMES_PERMIN         (CDFRAMES_PERSEC * 60)
#define FRAME_OF_ADDR(a)        ((a)[1] * CDFRAMES_PERMIN + (a)[2] * CDFRAMES_PERSEC + (a)[3])
#define FRAME_OF_TOC(toc, idx)  FRAME_OF_ADDR((toc)->TrackData[(idx) - (toc)->FirstTrack].Address)

#define GETWORD(buf,off)  MAKEWORD(buf[(off)],buf[(off+1)])
#define GETLONG(buf,off)  MAKELONG(GETWORD(buf,off),GETWORD(buf,off+2))

enum fs_type
{
    FS_ERROR,    /* error accessing the device */
    FS_UNKNOWN,  /* unknown file system */
    FS_FAT1216,
    FS_FAT32,
    FS_ISO9660,
    FS_UDF       /* For reference [E] = Ecma-167.pdf, [U] = udf260.pdf */
};

/* read a Unix symlink; returned buffer must be freed by caller */
static char *read_symlink( const char *path )
{
    char *buffer;
    int ret, size = 128;

    for (;;)
    {
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
        ret = readlink( path, buffer, size );
        if (ret == -1)
        {
            FILE_SetDosError();
            HeapFree( GetProcessHeap(), 0, buffer );
            return 0;
        }
        if (ret != size)
        {
            buffer[ret] = 0;
            return buffer;
        }
        HeapFree( GetProcessHeap(), 0, buffer );
        size *= 2;
    }
}

/* get the path of a dos device symlink in the $WINEPREFIX/dosdevices directory */
static char *get_dos_device_path( LPCWSTR name )
{
    const char *config_dir = wine_get_config_dir();
    char *buffer, *dev;
    int i;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0,
                              strlen(config_dir) + sizeof("/dosdevices/") + 5 )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    strcpy( buffer, config_dir );
    strcat( buffer, "/dosdevices/" );
    dev = buffer + strlen(buffer);
    /* no codepage conversion, DOS device names are ASCII anyway */
    for (i = 0; i < 5; i++)
        if (!(dev[i] = (char)tolowerW(name[i]))) break;
    dev[5] = 0;
    return buffer;
}

/* read the contents of an NT symlink object */
static NTSTATUS read_nt_symlink( const WCHAR *name, WCHAR *target, DWORD size )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE handle;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, name );

    if (!(status = NtOpenSymbolicLinkObject( &handle, SYMBOLIC_LINK_QUERY, &attr )))
    {
        UNICODE_STRING targetW;
        targetW.Buffer = target;
        targetW.MaximumLength = (size - 1) * sizeof(WCHAR);
        status = NtQuerySymbolicLinkObject( handle, &targetW, NULL );
        if (!status) target[targetW.Length / sizeof(WCHAR)] = 0;
        NtClose( handle );
    }
    return status;
}

/* open a handle to a device root */
static BOOL open_device_root( LPCWSTR root, HANDLE *handle )
{
    static const WCHAR default_rootW[] = {'\\',0};
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (!root) root = default_rootW;
    if (!RtlDosPathNameToNtPathName_U( root, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( handle, 0, &attr, &io, 0,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}

/* query the type of a drive from the mount manager */
static DWORD get_mountmgr_drive_type( LPCWSTR root )
{
    HANDLE mgr;
    struct mountmgr_unix_drive data;
    DWORD br;

    memset( &data, 0, sizeof(data) );
    if (root) data.letter = root[0];
    else
    {
        WCHAR curdir[MAX_PATH];
        GetCurrentDirectoryW( MAX_PATH, curdir );
        if (curdir[1] != ':' || curdir[2] != '\\') return DRIVE_UNKNOWN;
        data.letter = curdir[0];
    }

    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ,
                       FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return DRIVE_UNKNOWN;

    if (!DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE, &data, sizeof(data), &data,
                          sizeof(data), &br, NULL ) && GetLastError() != ERROR_MORE_DATA)
        data.type = DRIVE_UNKNOWN;

    CloseHandle( mgr );
    return data.type;
}

/* get the label by reading it from a file at the root of the filesystem */
static void get_filesystem_label( const UNICODE_STRING *device, WCHAR *label, DWORD len )
{
    static const WCHAR labelW[] = {'.','w','i','n','d','o','w','s','-','l','a','b','e','l',0};
    HANDLE handle;
    UNICODE_STRING name;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;

    label[0] = 0;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    name.MaximumLength = device->Length + sizeof(labelW);
    name.Length = name.MaximumLength - sizeof(WCHAR);
    if (!(name.Buffer = HeapAlloc( GetProcessHeap(), 0, name.MaximumLength ))) return;

    memcpy( name.Buffer, device->Buffer, device->Length );
    memcpy( name.Buffer + device->Length / sizeof(WCHAR), labelW, sizeof(labelW) );
    if (!NtOpenFile( &handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ|FILE_SHARE_WRITE,
                     FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT ))
    {
        char buffer[256], *p;
        DWORD size;

        if (!ReadFile( handle, buffer, sizeof(buffer)-1, &size, NULL )) size = 0;
        CloseHandle( handle );
        p = buffer + size;
        while (p > buffer && (p[-1] == ' ' || p[-1] == '\r' || p[-1] == '\n')) p--;
        *p = 0;
        if (!MultiByteToWideChar( CP_UNIXCP, 0, buffer, -1, label, len ))
            label[len-1] = 0;
    }
    RtlFreeUnicodeString( &name );
}

/* get the serial number by reading it from a file at the root of the filesystem */
static DWORD get_filesystem_serial( const UNICODE_STRING *device )
{
    static const WCHAR serialW[] = {'.','w','i','n','d','o','w','s','-','s','e','r','i','a','l',0};
    HANDLE handle;
    UNICODE_STRING name;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    DWORD ret = 0;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    name.MaximumLength = device->Length + sizeof(serialW);
    name.Length = name.MaximumLength - sizeof(WCHAR);
    if (!(name.Buffer = HeapAlloc( GetProcessHeap(), 0, name.MaximumLength ))) return 0;

    memcpy( name.Buffer, device->Buffer, device->Length );
    memcpy( name.Buffer + device->Length / sizeof(WCHAR), serialW, sizeof(serialW) );
    if (!NtOpenFile( &handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ|FILE_SHARE_WRITE,
                     FILE_SYNCHRONOUS_IO_NONALERT ))
    {
        char buffer[32];
        DWORD size;

        if (!ReadFile( handle, buffer, sizeof(buffer)-1, &size, NULL )) size = 0;
        CloseHandle( handle );
        buffer[size] = 0;
        ret = strtoul( buffer, NULL, 16 );
    }
    RtlFreeUnicodeString( &name );
    return ret;
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
     *  - an Extented Area (UDF) -- [E] 2/8.3.1 and [U] 2.1.7
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

    for(i=0; i<sizeof(locations)/sizeof(locations[0]); i++)
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
static void VOLUME_GetSuperblockLabel( const UNICODE_STRING *device, HANDLE handle,
                                       enum fs_type type, const BYTE *superblock,
                                       WCHAR *label, DWORD len )
{
    const BYTE *label_ptr = NULL;
    DWORD label_len;

    switch(type)
    {
    case FS_ERROR:
        label_len = 0;
        break;
    case FS_UNKNOWN:
        get_filesystem_label( device, label, len );
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

                if (len > 17) len = 17;
                for (i = 0; i < len-1; i++)
                    label[i] = (superblock[40+2*i] << 8) | superblock[41+2*i];
                label[i] = 0;
                while (i && label[i-1] == ' ') label[--i] = 0;
                return;
            }
            label_ptr = superblock + 40;
            label_len = 32;
            break;
        }
    case FS_UDF:
        {
            BYTE pvd[BLOCK_SIZE];

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
                for(i=0; i<label_len && i<len; i+=2)
                    label[i/2]  = (pvd[24+1 +i] << 8) | pvd[24+1 +i+1];
                label[label_len] = 0;
                return;
            }
        }
    }
    if (label_len) RtlMultiByteToUnicodeN( label, (len-1) * sizeof(WCHAR),
                                           &label_len, (LPCSTR)label_ptr, label_len );
    label_len /= sizeof(WCHAR);
    label[label_len] = 0;
    while (label_len && label[label_len-1] == ' ') label[--label_len] = 0;
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
static DWORD VOLUME_GetSuperblockSerial( const UNICODE_STRING *device, HANDLE handle,
                                         enum fs_type type, const BYTE *superblock )
{
    int FSD_sector;
    BYTE block[BLOCK_SIZE];

    switch(type)
    {
    case FS_ERROR:
        break;
    case FS_UNKNOWN:
        return get_filesystem_serial( device );
    case FS_FAT1216:
        return GETLONG( superblock, 0x27 );
    case FS_FAT32:
        return GETLONG( superblock, 0x33 );
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
            if ((GetVersion() & 0x80000000) || type == FS_UDF)
                return (sum[3] << 24) | (sum[2] << 16) | (sum[1] << 8) | sum[0];
            else
                return (sum[0] << 24) | (sum[1] << 16) | (sum[2] << 8) | sum[3];
        }
    }
    return 0;
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


/***********************************************************************
 *           GetVolumeInformationW   (KERNEL32.@)
 */
BOOL WINAPI GetVolumeInformationW( LPCWSTR root, LPWSTR label, DWORD label_len,
                                   DWORD *serial, DWORD *filename_len, DWORD *flags,
                                   LPWSTR fsname, DWORD fsname_len )
{
    static const WCHAR audiocdW[] = {'A','u','d','i','o',' ','C','D',0};
    static const WCHAR fatW[] = {'F','A','T',0};
    static const WCHAR fat32W[] = {'F','A','T','3','2',0};
    static const WCHAR ntfsW[] = {'N','T','F','S',0};
    static const WCHAR cdfsW[] = {'C','D','F','S',0};
    static const WCHAR udfW[] = {'U','D','F',0};
    static const WCHAR default_rootW[] = {'\\',0};

    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING nt_name;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    FILE_FS_DEVICE_INFORMATION info;
    WCHAR *p;
    enum fs_type type = FS_UNKNOWN;
    BOOL ret = FALSE;

    if (!root) root = default_rootW;
    if (!RtlDosPathNameToNtPathName_U( root, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    /* there must be exactly one backslash in the name, at the end */
    p = memchrW( nt_name.Buffer + 4, '\\', (nt_name.Length - 4) / sizeof(WCHAR) );
    if (p != nt_name.Buffer + nt_name.Length / sizeof(WCHAR) - 1)
    {
        /* check if root contains an explicit subdir */
        if (root[0] && root[1] == ':') root += 2;
        while (*root == '\\') root++;
        if (strchrW( root, '\\' ))
            SetLastError( ERROR_DIR_NOT_ROOT );
        else
            SetLastError( ERROR_INVALID_NAME );
        goto done;
    }

    /* try to open the device */

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    nt_name.Length -= sizeof(WCHAR);  /* without trailing slash */
    status = NtOpenFile( &handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    nt_name.Length += sizeof(WCHAR);

    if (status == STATUS_SUCCESS)
    {
        BYTE superblock[SUPERBLOCK_SIZE];
        CDROM_TOC toc;
        DWORD br;

        /* check for audio CD */
        /* FIXME: we only check the first track for now */
        if (DeviceIoControl( handle, IOCTL_CDROM_READ_TOC, NULL, 0, &toc, sizeof(toc), &br, 0 ))
        {
            if (!(toc.TrackData[0].Control & 0x04))  /* audio track */
            {
                TRACE( "%s: found audio CD\n", debugstr_w(nt_name.Buffer) );
                if (label) lstrcpynW( label, audiocdW, label_len );
                if (serial) *serial = VOLUME_GetAudioCDSerial( &toc );
                CloseHandle( handle );
                type = FS_ISO9660;
                goto fill_fs_info;
            }
            type = VOLUME_ReadCDSuperblock( handle, superblock );
        }
        else
        {
            type = VOLUME_ReadFATSuperblock( handle, superblock );
            if (type == FS_UNKNOWN) type = VOLUME_ReadCDSuperblock( handle, superblock );
        }
        TRACE( "%s: found fs type %d\n", debugstr_w(nt_name.Buffer), type );
        if (type == FS_ERROR)
        {
            CloseHandle( handle );
            goto done;
        }

        if (label && label_len) VOLUME_GetSuperblockLabel( &nt_name, handle, type, superblock, label, label_len );
        if (serial) *serial = VOLUME_GetSuperblockSerial( &nt_name, handle, type, superblock );
        CloseHandle( handle );
        goto fill_fs_info;
    }
    else TRACE( "cannot open device %s: %x\n", debugstr_w(nt_name.Buffer), status );

    /* we couldn't open the device, fallback to default strategy */

    status = NtOpenFile( &handle, 0, &attr, &io, 0, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        goto done;
    }
    status = NtQueryVolumeInformationFile( handle, &io, &info, sizeof(info), FileFsDeviceInformation );
    NtClose( handle );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        goto done;
    }
    if (info.DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM) type = FS_ISO9660;

    if (label && label_len) get_filesystem_label( &nt_name, label, label_len );
    if (serial) *serial = get_filesystem_serial( &nt_name );

fill_fs_info:  /* now fill in the information that depends on the file system type */

    switch(type)
    {
    case FS_ISO9660:
        if (fsname) lstrcpynW( fsname, cdfsW, fsname_len );
        if (filename_len) *filename_len = 221;
        if (flags) *flags = FILE_READ_ONLY_VOLUME;
        break;
    case FS_UDF:
        if (fsname) lstrcpynW( fsname, udfW, fsname_len );
        if (filename_len) *filename_len = 255;
        if (flags)
            *flags = FILE_READ_ONLY_VOLUME | FILE_UNICODE_ON_DISK | FILE_CASE_SENSITIVE_SEARCH;
        break;
    case FS_FAT1216:
        if (fsname) lstrcpynW( fsname, fatW, fsname_len );
    case FS_FAT32:
        if (type == FS_FAT32 && fsname) lstrcpynW( fsname, fat32W, fsname_len );
        if (filename_len) *filename_len = 255;
        if (flags) *flags = FILE_CASE_PRESERVED_NAMES;  /* FIXME */
        break;
    default:
        if (fsname) lstrcpynW( fsname, ntfsW, fsname_len );
        if (filename_len) *filename_len = 255;
        if (flags) *flags = FILE_CASE_PRESERVED_NAMES | FILE_PERSISTENT_ACLS;
        break;
    }
    ret = TRUE;

done:
    RtlFreeUnicodeString( &nt_name );
    return ret;
}


/***********************************************************************
 *           GetVolumeInformationA   (KERNEL32.@)
 */
BOOL WINAPI GetVolumeInformationA( LPCSTR root, LPSTR label,
                                   DWORD label_len, DWORD *serial,
                                   DWORD *filename_len, DWORD *flags,
                                   LPSTR fsname, DWORD fsname_len )
{
    WCHAR *rootW = NULL;
    LPWSTR labelW, fsnameW;
    BOOL ret;

    if (root && !(rootW = FILE_name_AtoW( root, FALSE ))) return FALSE;

    labelW = label ? HeapAlloc(GetProcessHeap(), 0, label_len * sizeof(WCHAR)) : NULL;
    fsnameW = fsname ? HeapAlloc(GetProcessHeap(), 0, fsname_len * sizeof(WCHAR)) : NULL;

    if ((ret = GetVolumeInformationW(rootW, labelW, label_len, serial,
                                    filename_len, flags, fsnameW, fsname_len)))
    {
        if (label) FILE_name_WtoA( labelW, -1, label, label_len );
        if (fsname) FILE_name_WtoA( fsnameW, -1, fsname, fsname_len );
    }

    HeapFree( GetProcessHeap(), 0, labelW );
    HeapFree( GetProcessHeap(), 0, fsnameW );
    return ret;
}



/***********************************************************************
 *           SetVolumeLabelW   (KERNEL32.@)
 */
BOOL WINAPI SetVolumeLabelW( LPCWSTR root, LPCWSTR label )
{
    WCHAR device[] = {'\\','\\','.','\\','A',':',0};
    HANDLE handle;
    enum fs_type type = FS_UNKNOWN;

    if (!root)
    {
        WCHAR path[MAX_PATH];
        GetCurrentDirectoryW( MAX_PATH, path );
        device[4] = path[0];
    }
    else
    {
        if (!root[0] || root[1] != ':')
        {
            SetLastError( ERROR_INVALID_NAME );
            return FALSE;
        }
        device[4] = root[0];
    }

    /* try to open the device */

    handle = CreateFileW( device, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, 0 );
    if (handle != INVALID_HANDLE_VALUE)
    {
        BYTE superblock[SUPERBLOCK_SIZE];

        type = VOLUME_ReadFATSuperblock( handle, superblock );
        if (type == FS_UNKNOWN) type = VOLUME_ReadCDSuperblock( handle, superblock );
        CloseHandle( handle );
        if (type != FS_UNKNOWN)
        {
            /* we can't set the label on FAT or CDROM file systems */
            TRACE( "cannot set label on device %s type %d\n", debugstr_w(device), type );
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }
    }
    else
    {
        TRACE( "cannot open device %s: err %d\n", debugstr_w(device), GetLastError() );
        if (GetLastError() == ERROR_ACCESS_DENIED) return FALSE;
    }

    /* we couldn't open the device, fallback to default strategy */

    switch(GetDriveTypeW( root ))
    {
    case DRIVE_UNKNOWN:
    case DRIVE_NO_ROOT_DIR:
        SetLastError( ERROR_NOT_READY );
        break;
    case DRIVE_REMOVABLE:
    case DRIVE_FIXED:
        {
            WCHAR labelW[] = {'A',':','\\','.','w','i','n','d','o','w','s','-','l','a','b','e','l',0};

            labelW[0] = device[4];

            if (!label[0])  /* delete label file when setting an empty label */
                return DeleteFileW( labelW ) || GetLastError() == ERROR_FILE_NOT_FOUND;

            handle = CreateFileW( labelW, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                  CREATE_ALWAYS, 0, 0 );
            if (handle != INVALID_HANDLE_VALUE)
            {
                char buffer[64];
                DWORD size;

                if (!WideCharToMultiByte( CP_UNIXCP, 0, label, -1, buffer, sizeof(buffer)-1, NULL, NULL ))
                    buffer[sizeof(buffer)-2] = 0;
                strcat( buffer, "\n" );
                WriteFile( handle, buffer, strlen(buffer), &size, NULL );
                CloseHandle( handle );
                return TRUE;
            }
            break;
        }
    case DRIVE_REMOTE:
    case DRIVE_RAMDISK:
    case DRIVE_CDROM:
        SetLastError( ERROR_ACCESS_DENIED );
        break;
    }
    return FALSE;
}

/***********************************************************************
 *           SetVolumeLabelA   (KERNEL32.@)
 */
BOOL WINAPI SetVolumeLabelA(LPCSTR root, LPCSTR volname)
{
    WCHAR *rootW = NULL, *volnameW = NULL;
    BOOL ret;

    if (root && !(rootW = FILE_name_AtoW( root, FALSE ))) return FALSE;
    if (volname && !(volnameW = FILE_name_AtoW( volname, TRUE ))) return FALSE;
    ret = SetVolumeLabelW( rootW, volnameW );
    HeapFree( GetProcessHeap(), 0, volnameW );
    return ret;
}


/***********************************************************************
 *           GetVolumeNameForVolumeMountPointA   (KERNEL32.@)
 */
BOOL WINAPI GetVolumeNameForVolumeMountPointA( LPCSTR path, LPSTR volume, DWORD size )
{
    BOOL ret;
    WCHAR volumeW[50], *pathW = NULL;
    DWORD len = min( sizeof(volumeW) / sizeof(WCHAR), size );

    TRACE("(%s, %p, %x)\n", debugstr_a(path), volume, size);

    if (!path || !(pathW = FILE_name_AtoW( path, TRUE )))
        return FALSE;

    if ((ret = GetVolumeNameForVolumeMountPointW( pathW, volumeW, len )))
        FILE_name_WtoA( volumeW, -1, volume, len );

    HeapFree( GetProcessHeap(), 0, pathW );
    return ret;
}

/***********************************************************************
 *           GetVolumeNameForVolumeMountPointW   (KERNEL32.@)
 */
BOOL WINAPI GetVolumeNameForVolumeMountPointW( LPCWSTR path, LPWSTR volume, DWORD size )
{
    static const WCHAR prefixW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};
    static const WCHAR volumeW[] = {'\\','?','?','\\','V','o','l','u','m','e','{'};
    static const WCHAR trailingW[] = {'\\',0};

    MOUNTMGR_MOUNT_POINT *input = NULL, *o1;
    MOUNTMGR_MOUNT_POINTS *output = NULL;
    WCHAR *p;
    char *r;
    DWORD i, i_size = 1024, o_size = 1024;
    WCHAR *nonpersist_name;
    WCHAR symlink_name[MAX_PATH];
    NTSTATUS status;
    HANDLE mgr = INVALID_HANDLE_VALUE;
    BOOL ret = FALSE;
    DWORD br;

    TRACE("(%s, %p, %x)\n", debugstr_w(path), volume, size);
    if (path[lstrlenW(path)-1] != '\\')
    {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }

    if (size < 50)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return FALSE;
    }
    /* if length of input is > 3 then it must be a mounted folder */
    if (lstrlenW(path) > 3)
    {
        FIXME("Mounted Folders are not yet supported\n");
        SetLastError( ERROR_NOT_A_REPARSE_POINT );
        return FALSE;
    }

    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return FALSE;

    if (!(input = HeapAlloc( GetProcessHeap(), 0, i_size )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto err_ret;
    }

    if (!(output = HeapAlloc( GetProcessHeap(), 0, o_size )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto err_ret;
    }

    /* construct the symlink name as "\DosDevices\C:" */
    lstrcpyW( symlink_name, prefixW );
    lstrcatW( symlink_name, path );
    symlink_name[lstrlenW(symlink_name)-1] = 0;

    /* Take the mount point and get the "nonpersistent name" */
    /* We will then take that and get the volume name        */
    nonpersist_name = (WCHAR *)(input + 1);
    status = read_nt_symlink( symlink_name, nonpersist_name, i_size - sizeof(*input) );
    TRACE("read_nt_symlink got stat=%x, for %s, got <%s>\n", status,
            debugstr_w(symlink_name), debugstr_w(nonpersist_name));
    if (status != STATUS_SUCCESS)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        goto err_ret;
    }

    /* Now take the "nonpersistent name" and ask the mountmgr  */
    /* to give us all the mount points.  One of them will be   */
    /* the volume name  (format of \??\Volume{).               */
    memset( input, 0, sizeof(*input) );  /* clear all input parameters */
    input->DeviceNameOffset = sizeof(*input);
    input->DeviceNameLength = lstrlenW( nonpersist_name) * sizeof(WCHAR);
    i_size = input->DeviceNameOffset + input->DeviceNameLength;

    output->Size = o_size;

    /* now get the true volume name from the mountmgr   */
    if (!DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_POINTS, input, i_size,
                        output, o_size, &br, NULL ))
        goto err_ret;

    /* Verify and return the data, note string is not null terminated  */
    TRACE("found %d matching mount points\n", output->NumberOfMountPoints);
    if (output->NumberOfMountPoints < 1)
    {
        SetLastError( ERROR_NO_VOLUME_ID );
        goto err_ret;
    }
    o1 = &output->MountPoints[0];

    /* look for the volume name in returned values  */
    for(i=0;i<output->NumberOfMountPoints;i++)
    {
        p = (WCHAR*)((char *)output + o1->SymbolicLinkNameOffset);
        r = (char *)output + o1->UniqueIdOffset;
        TRACE("found symlink=%s, unique=%s, devname=%s\n",
            debugstr_wn(p, o1->SymbolicLinkNameLength/sizeof(WCHAR)),
            debugstr_an(r, o1->UniqueIdLength),
            debugstr_wn((WCHAR*)((char *)output + o1->DeviceNameOffset),
                            o1->DeviceNameLength/sizeof(WCHAR)));

        if (!strncmpW( p, volumeW, sizeof(volumeW)/sizeof(WCHAR) ))
        {
            /* is there space in the return variable ?? */
            if ((o1->SymbolicLinkNameLength/sizeof(WCHAR))+2 > size)
            {
                SetLastError( ERROR_FILENAME_EXCED_RANGE );
                goto err_ret;
            }
            memcpy( volume, p, o1->SymbolicLinkNameLength );
            volume[o1->SymbolicLinkNameLength / sizeof(WCHAR)] = 0;
            lstrcatW( volume, trailingW );
            /* change second char from '?' to '\'  */
            volume[1] = '\\';
            ret = TRUE;
            break;
        }
        o1++;
    }

err_ret:
    HeapFree( GetProcessHeap(), 0, input );
    HeapFree( GetProcessHeap(), 0, output );
    CloseHandle( mgr );
    return ret;
}

/***********************************************************************
 *           DefineDosDeviceW       (KERNEL32.@)
 */
BOOL WINAPI DefineDosDeviceW( DWORD flags, LPCWSTR devname, LPCWSTR targetpath )
{
    DWORD len, dosdev;
    BOOL ret = FALSE;
    char *path = NULL, *target, *p;

    TRACE("%x, %s, %s\n", flags, debugstr_w(devname), debugstr_w(targetpath));

    if (!(flags & DDD_REMOVE_DEFINITION))
    {
        if (!(flags & DDD_RAW_TARGET_PATH))
        {
            FIXME( "(0x%08x,%s,%s) DDD_RAW_TARGET_PATH flag not set, not supported yet\n",
                   flags, debugstr_w(devname), debugstr_w(targetpath) );
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return FALSE;
        }

        len = WideCharToMultiByte( CP_UNIXCP, 0, targetpath, -1, NULL, 0, NULL, NULL );
        if ((target = HeapAlloc( GetProcessHeap(), 0, len )))
        {
            WideCharToMultiByte( CP_UNIXCP, 0, targetpath, -1, target, len, NULL, NULL );
            for (p = target; *p; p++) if (*p == '\\') *p = '/';
        }
        else
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
    }
    else target = NULL;

    /* first check for a DOS device */

    if ((dosdev = RtlIsDosDeviceName_U( devname )))
    {
        WCHAR name[5];

        memcpy( name, devname + HIWORD(dosdev)/sizeof(WCHAR), LOWORD(dosdev) );
        name[LOWORD(dosdev)/sizeof(WCHAR)] = 0;
        path = get_dos_device_path( name );
    }
    else if (isalphaW(devname[0]) && devname[1] == ':' && !devname[2])  /* drive mapping */
    {
        path = get_dos_device_path( devname );
    }
    else SetLastError( ERROR_FILE_NOT_FOUND );

    if (path)
    {
        if (target)
        {
            TRACE( "creating symlink %s -> %s\n", path, target );
            unlink( path );
            if (!symlink( target, path )) ret = TRUE;
            else FILE_SetDosError();
        }
        else
        {
            TRACE( "removing symlink %s\n", path );
            if (!unlink( path )) ret = TRUE;
            else FILE_SetDosError();
        }
        HeapFree( GetProcessHeap(), 0, path );
    }
    HeapFree( GetProcessHeap(), 0, target );
    return ret;
}


/***********************************************************************
 *           DefineDosDeviceA       (KERNEL32.@)
 */
BOOL WINAPI DefineDosDeviceA(DWORD flags, LPCSTR devname, LPCSTR targetpath)
{
    WCHAR *devW, *targetW = NULL;
    BOOL ret;

    if (!(devW = FILE_name_AtoW( devname, FALSE ))) return FALSE;
    if (targetpath && !(targetW = FILE_name_AtoW( targetpath, TRUE ))) return FALSE;
    ret = DefineDosDeviceW(flags, devW, targetW);
    HeapFree( GetProcessHeap(), 0, targetW );
    return ret;
}


/***********************************************************************
 *           QueryDosDeviceW   (KERNEL32.@)
 *
 * returns array of strings terminated by \0, terminated by \0
 */
DWORD WINAPI QueryDosDeviceW( LPCWSTR devname, LPWSTR target, DWORD bufsize )
{
    static const WCHAR auxW[] = {'A','U','X',0};
    static const WCHAR nulW[] = {'N','U','L',0};
    static const WCHAR prnW[] = {'P','R','N',0};
    static const WCHAR comW[] = {'C','O','M',0};
    static const WCHAR lptW[] = {'L','P','T',0};
    static const WCHAR com0W[] = {'\\','?','?','\\','C','O','M','0',0};
    static const WCHAR com1W[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','C','O','M','1',0,0};
    static const WCHAR lpt1W[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','L','P','T','1',0,0};
    static const WCHAR dosdevW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};

    UNICODE_STRING nt_name;
    ANSI_STRING unix_name;
    WCHAR nt_buffer[10];
    NTSTATUS status;

    if (!bufsize)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (devname)
    {
        WCHAR *p, name[5];
        char *path, *link;
        DWORD dosdev, ret = 0;

        if ((dosdev = RtlIsDosDeviceName_U( devname )))
        {
            memcpy( name, devname + HIWORD(dosdev)/sizeof(WCHAR), LOWORD(dosdev) );
            name[LOWORD(dosdev)/sizeof(WCHAR)] = 0;
        }
        else
        {
            WCHAR *buffer;

            if (!(buffer = HeapAlloc( GetProcessHeap(), 0, sizeof(dosdevW) + strlenW(devname)*sizeof(WCHAR) )))
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }
            memcpy( buffer, dosdevW, sizeof(dosdevW) );
            strcatW( buffer, devname );
            status = read_nt_symlink( buffer, target, bufsize );
            HeapFree( GetProcessHeap(), 0, buffer );
            if (status)
            {
                SetLastError( RtlNtStatusToDosError(status) );
                return 0;
            }
            ret = strlenW( target ) + 1;
            goto done;
        }

        /* FIXME: should read NT symlink for all devices */

        if (!(path = get_dos_device_path( name ))) return 0;
        link = read_symlink( path );
        HeapFree( GetProcessHeap(), 0, path );

        if (link)
        {
            ret = MultiByteToWideChar( CP_UNIXCP, 0, link, -1, target, bufsize );
            HeapFree( GetProcessHeap(), 0, link );
        }
        else if (dosdev)  /* look for device defaults */
        {
            if (!strcmpiW( name, auxW ))
            {
                if (bufsize >= sizeof(com1W)/sizeof(WCHAR))
                {
                    memcpy( target, com1W, sizeof(com1W) );
                    ret = sizeof(com1W)/sizeof(WCHAR);
                }
                else SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return ret;
            }
            if (!strcmpiW( name, prnW ))
            {
                if (bufsize >= sizeof(lpt1W)/sizeof(WCHAR))
                {
                    memcpy( target, lpt1W, sizeof(lpt1W) );
                    ret = sizeof(lpt1W)/sizeof(WCHAR);
                }
                else SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return ret;
            }

            nt_buffer[0] = '\\';
            nt_buffer[1] = '?';
            nt_buffer[2] = '?';
            nt_buffer[3] = '\\';
            strcpyW( nt_buffer + 4, name );
            RtlInitUnicodeString( &nt_name, nt_buffer );
            status = wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, TRUE );
            if (status) SetLastError( RtlNtStatusToDosError(status) );
            else
            {
                ret = MultiByteToWideChar( CP_UNIXCP, 0, unix_name.Buffer, -1, target, bufsize );
                RtlFreeAnsiString( &unix_name );
            }
        }
    done:
        if (ret)
        {
            if (ret < bufsize) target[ret++] = 0;  /* add an extra null */
            for (p = target; *p; p++) if (*p == '/') *p = '\\';
        }

        return ret;
    }
    else  /* return a list of all devices */
    {
        OBJECT_ATTRIBUTES attr;
        HANDLE handle;
        WCHAR *p = target;
        int i;

        if (bufsize <= (sizeof(auxW)+sizeof(nulW)+sizeof(prnW))/sizeof(WCHAR))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }

        /* FIXME: these should be NT symlinks too */

        memcpy( p, auxW, sizeof(auxW) );
        p += sizeof(auxW) / sizeof(WCHAR);
        memcpy( p, nulW, sizeof(nulW) );
        p += sizeof(nulW) / sizeof(WCHAR);
        memcpy( p, prnW, sizeof(prnW) );
        p += sizeof(prnW) / sizeof(WCHAR);

        strcpyW( nt_buffer, com0W );
        RtlInitUnicodeString( &nt_name, nt_buffer );

        for (i = 1; i <= 9; i++)
        {
            nt_buffer[7] = '0' + i;
            if (!wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, TRUE ))
            {
                RtlFreeAnsiString( &unix_name );
                if (p + 5 >= target + bufsize)
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    return 0;
                }
                strcpyW( p, comW );
                p[3] = '0' + i;
                p[4] = 0;
                p += 5;
            }
        }
        strcpyW( nt_buffer + 4, lptW );
        for (i = 1; i <= 9; i++)
        {
            nt_buffer[7] = '0' + i;
            if (!wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, TRUE ))
            {
                RtlFreeAnsiString( &unix_name );
                if (p + 5 >= target + bufsize)
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    return 0;
                }
                strcpyW( p, lptW );
                p[3] = '0' + i;
                p[4] = 0;
                p += 5;
            }
        }

        RtlInitUnicodeString( &nt_name, dosdevW );
        nt_name.Length -= sizeof(WCHAR);  /* without trailing slash */
        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &nt_name;
        attr.Attributes = OBJ_CASE_INSENSITIVE;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        status = NtOpenDirectoryObject( &handle, FILE_LIST_DIRECTORY, &attr );
        if (!status)
        {
            char data[1024];
            DIRECTORY_BASIC_INFORMATION *info = (DIRECTORY_BASIC_INFORMATION *)data;
            ULONG ctx = 0, len;

            while (!NtQueryDirectoryObject( handle, info, sizeof(data), 1, 0, &ctx, &len ))
            {
                if (p + info->ObjectName.Length/sizeof(WCHAR) + 1 >= target + bufsize)
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    NtClose( handle );
                    return 0;
                }
                memcpy( p, info->ObjectName.Buffer, info->ObjectName.Length );
                p += info->ObjectName.Length/sizeof(WCHAR);
                *p++ = 0;
            }
            NtClose( handle );
        }

        *p++ = 0;  /* terminating null */
        return p - target;
    }
}


/***********************************************************************
 *           QueryDosDeviceA   (KERNEL32.@)
 *
 * returns array of strings terminated by \0, terminated by \0
 */
DWORD WINAPI QueryDosDeviceA( LPCSTR devname, LPSTR target, DWORD bufsize )
{
    DWORD ret = 0, retW;
    WCHAR *devnameW = NULL;
    LPWSTR targetW;

    if (devname && !(devnameW = FILE_name_AtoW( devname, FALSE ))) return 0;

    targetW = HeapAlloc( GetProcessHeap(),0, bufsize * sizeof(WCHAR) );
    if (!targetW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    retW = QueryDosDeviceW(devnameW, targetW, bufsize);

    ret = FILE_name_WtoA( targetW, retW, target, bufsize );

    HeapFree(GetProcessHeap(), 0, targetW);
    return ret;
}


/***********************************************************************
 *           GetLogicalDrives   (KERNEL32.@)
 */
DWORD WINAPI GetLogicalDrives(void)
{
    static const WCHAR dosdevW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nt_name;
    DWORD bitmask = 0;
    NTSTATUS status;
    HANDLE handle;

    RtlInitUnicodeString( &nt_name, dosdevW );
    nt_name.Length -= sizeof(WCHAR);  /* without trailing slash */
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nt_name;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    status = NtOpenDirectoryObject( &handle, FILE_LIST_DIRECTORY, &attr );
    if (!status)
    {
        char data[1024];
        DIRECTORY_BASIC_INFORMATION *info = (DIRECTORY_BASIC_INFORMATION *)data;
        ULONG ctx = 0, len;

        while (!NtQueryDirectoryObject( handle, info, sizeof(data), 1, 0, &ctx, &len ))
            if(info->ObjectName.Length == 2*sizeof(WCHAR) && info->ObjectName.Buffer[1] == ':')
                bitmask |= 1 << (info->ObjectName.Buffer[0] - 'A');

        NtClose( handle );
    }

    return bitmask;
}


/***********************************************************************
 *           GetLogicalDriveStringsA   (KERNEL32.@)
 */
UINT WINAPI GetLogicalDriveStringsA( UINT len, LPSTR buffer )
{
    DWORD drives = GetLogicalDrives();
    UINT drive, count;

    for (drive = count = 0; drive < 26; drive++) if (drives & (1 << drive)) count++;
    if ((count * 4) + 1 > len) return count * 4 + 1;

    for (drive = 0; drive < 26; drive++)
    {
        if (drives & (1 << drive))
        {
            *buffer++ = 'A' + drive;
            *buffer++ = ':';
            *buffer++ = '\\';
            *buffer++ = 0;
        }
    }
    *buffer = 0;
    return count * 4;
}


/***********************************************************************
 *           GetLogicalDriveStringsW   (KERNEL32.@)
 */
UINT WINAPI GetLogicalDriveStringsW( UINT len, LPWSTR buffer )
{
    DWORD drives = GetLogicalDrives();
    UINT drive, count;

    for (drive = count = 0; drive < 26; drive++) if (drives & (1 << drive)) count++;
    if ((count * 4) + 1 > len) return count * 4 + 1;

    for (drive = 0; drive < 26; drive++)
    {
        if (drives & (1 << drive))
        {
            *buffer++ = 'A' + drive;
            *buffer++ = ':';
            *buffer++ = '\\';
            *buffer++ = 0;
        }
    }
    *buffer = 0;
    return count * 4;
}


/***********************************************************************
 *           GetDriveTypeW   (KERNEL32.@)
 *
 * Returns the type of the disk drive specified. If root is NULL the
 * root of the current directory is used.
 *
 * RETURNS
 *
 *  Type of drive (from Win32 SDK):
 *
 *   DRIVE_UNKNOWN     unable to find out anything about the drive
 *   DRIVE_NO_ROOT_DIR nonexistent root dir
 *   DRIVE_REMOVABLE   the disk can be removed from the machine
 *   DRIVE_FIXED       the disk cannot be removed from the machine
 *   DRIVE_REMOTE      network disk
 *   DRIVE_CDROM       CDROM drive
 *   DRIVE_RAMDISK     virtual disk in RAM
 */
UINT WINAPI GetDriveTypeW(LPCWSTR root) /* [in] String describing drive */
{
    FILE_FS_DEVICE_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    UINT ret;

    if (!open_device_root( root, &handle )) return DRIVE_NO_ROOT_DIR;

    status = NtQueryVolumeInformationFile( handle, &io, &info, sizeof(info), FileFsDeviceInformation );
    NtClose( handle );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        ret = DRIVE_UNKNOWN;
    }
    else
    {
        switch (info.DeviceType)
        {
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:  ret = DRIVE_CDROM; break;
        case FILE_DEVICE_VIRTUAL_DISK:        ret = DRIVE_RAMDISK; break;
        case FILE_DEVICE_NETWORK_FILE_SYSTEM: ret = DRIVE_REMOTE; break;
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            if (info.Characteristics & FILE_REMOTE_DEVICE) ret = DRIVE_REMOTE;
            else if (info.Characteristics & FILE_REMOVABLE_MEDIA) ret = DRIVE_REMOVABLE;
            else if ((ret = get_mountmgr_drive_type( root )) == DRIVE_UNKNOWN) ret = DRIVE_FIXED;
            break;
        default:
            ret = DRIVE_UNKNOWN;
            break;
        }
    }
    TRACE( "%s -> %d\n", debugstr_w(root), ret );
    return ret;
}


/***********************************************************************
 *           GetDriveTypeA   (KERNEL32.@)
 *
 * See GetDriveTypeW.
 */
UINT WINAPI GetDriveTypeA( LPCSTR root )
{
    WCHAR *rootW = NULL;

    if (root && !(rootW = FILE_name_AtoW( root, FALSE ))) return DRIVE_NO_ROOT_DIR;
    return GetDriveTypeW( rootW );
}


/***********************************************************************
 *           GetDiskFreeSpaceExW   (KERNEL32.@)
 *
 *  This function is used to acquire the size of the available and
 *  total space on a logical volume.
 *
 * RETURNS
 *
 *  Zero on failure, nonzero upon success. Use GetLastError to obtain
 *  detailed error information.
 *
 */
BOOL WINAPI GetDiskFreeSpaceExW( LPCWSTR root, PULARGE_INTEGER avail,
                                 PULARGE_INTEGER total, PULARGE_INTEGER totalfree )
{
    FILE_FS_SIZE_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    UINT units;

    TRACE( "%s,%p,%p,%p\n", debugstr_w(root), avail, total, totalfree );

    if (!open_device_root( root, &handle )) return FALSE;

    status = NtQueryVolumeInformationFile( handle, &io, &info, sizeof(info), FileFsSizeInformation );
    NtClose( handle );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    units = info.SectorsPerAllocationUnit * info.BytesPerSector;
    if (total) total->QuadPart = info.TotalAllocationUnits.QuadPart * units;
    if (totalfree) totalfree->QuadPart = info.AvailableAllocationUnits.QuadPart * units;
    /* FIXME: this one should take quotas into account */
    if (avail) avail->QuadPart = info.AvailableAllocationUnits.QuadPart * units;
    return TRUE;
}


/***********************************************************************
 *           GetDiskFreeSpaceExA   (KERNEL32.@)
 *
 * See GetDiskFreeSpaceExW.
 */
BOOL WINAPI GetDiskFreeSpaceExA( LPCSTR root, PULARGE_INTEGER avail,
                                 PULARGE_INTEGER total, PULARGE_INTEGER totalfree )
{
    WCHAR *rootW = NULL;

    if (root && !(rootW = FILE_name_AtoW( root, FALSE ))) return FALSE;
    return GetDiskFreeSpaceExW( rootW, avail, total, totalfree );
}


/***********************************************************************
 *           GetDiskFreeSpaceW   (KERNEL32.@)
 */
BOOL WINAPI GetDiskFreeSpaceW( LPCWSTR root, LPDWORD cluster_sectors,
                               LPDWORD sector_bytes, LPDWORD free_clusters,
                               LPDWORD total_clusters )
{
    FILE_FS_SIZE_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    UINT units;

    TRACE( "%s,%p,%p,%p,%p\n", debugstr_w(root),
           cluster_sectors, sector_bytes, free_clusters, total_clusters );

    if (!open_device_root( root, &handle )) return FALSE;

    status = NtQueryVolumeInformationFile( handle, &io, &info, sizeof(info), FileFsSizeInformation );
    NtClose( handle );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    units = info.SectorsPerAllocationUnit * info.BytesPerSector;

    if( GetVersion() & 0x80000000) {    /* win3.x, 9x, ME */
        /* cap the size and available at 2GB as per specs */
        if (info.TotalAllocationUnits.QuadPart * units > 0x7fffffff) {
            info.TotalAllocationUnits.QuadPart = 0x7fffffff / units;
            if (info.AvailableAllocationUnits.QuadPart * units > 0x7fffffff)
                info.AvailableAllocationUnits.QuadPart = 0x7fffffff / units;
        }
        /* nr. of clusters is always <= 65335 */
        while( info.TotalAllocationUnits.QuadPart > 65535 ) {
            info.TotalAllocationUnits.QuadPart /= 2;
            info.AvailableAllocationUnits.QuadPart /= 2;
            info.SectorsPerAllocationUnit *= 2;
        }
    }

    if (cluster_sectors) *cluster_sectors = info.SectorsPerAllocationUnit;
    if (sector_bytes) *sector_bytes = info.BytesPerSector;
    if (free_clusters) *free_clusters = info.AvailableAllocationUnits.u.LowPart;
    if (total_clusters) *total_clusters = info.TotalAllocationUnits.u.LowPart;
    TRACE("%#08x, %#08x, %#08x, %#08x\n", info.SectorsPerAllocationUnit, info.BytesPerSector,
          info.AvailableAllocationUnits.u.LowPart, info.TotalAllocationUnits.u.LowPart);
    return TRUE;
}


/***********************************************************************
 *           GetDiskFreeSpaceA   (KERNEL32.@)
 */
BOOL WINAPI GetDiskFreeSpaceA( LPCSTR root, LPDWORD cluster_sectors,
                               LPDWORD sector_bytes, LPDWORD free_clusters,
                               LPDWORD total_clusters )
{
    WCHAR *rootW = NULL;

    if (root && !(rootW = FILE_name_AtoW( root, FALSE ))) return FALSE;
    return GetDiskFreeSpaceW( rootW, cluster_sectors, sector_bytes, free_clusters, total_clusters );
}

/***********************************************************************
 *           GetVolumePathNameA   (KERNEL32.@)
 */
BOOL WINAPI GetVolumePathNameA(LPCSTR filename, LPSTR volumepathname, DWORD buflen)
{
    BOOL ret;
    WCHAR *filenameW = NULL, *volumeW = NULL;

    FIXME("(%s, %p, %d), stub!\n", debugstr_a(filename), volumepathname, buflen);

    if (filename && !(filenameW = FILE_name_AtoW( filename, FALSE )))
        return FALSE;
    if (volumepathname && !(volumeW = HeapAlloc( GetProcessHeap(), 0, buflen * sizeof(WCHAR) )))
        return FALSE;

    if ((ret = GetVolumePathNameW( filenameW, volumeW, buflen )))
        FILE_name_WtoA( volumeW, -1, volumepathname, buflen );

    HeapFree( GetProcessHeap(), 0, volumeW );
    return ret;
}

/***********************************************************************
 *           GetVolumePathNameW   (KERNEL32.@)
 */
BOOL WINAPI GetVolumePathNameW(LPCWSTR filename, LPWSTR volumepathname, DWORD buflen)
{
    const WCHAR *p = filename;

    FIXME("(%s, %p, %d), stub!\n", debugstr_w(filename), volumepathname, buflen);

    if (!filename || !volumepathname || !buflen)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (p && tolowerW(p[0]) >= 'a' && tolowerW(p[0]) <= 'z' && p[1] ==':' && p[2] == '\\')
    {
        if (buflen < 4)
        {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
        volumepathname[0] = p[0];
        volumepathname[1] = ':';
        volumepathname[2] = '\\';
        volumepathname[3] = 0;
        return TRUE;
    }

    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
}

/***********************************************************************
 *           GetVolumePathNamesForVolumeNameA   (KERNEL32.@)
 */
BOOL WINAPI GetVolumePathNamesForVolumeNameA(LPCSTR volumename, LPSTR volumepathname, DWORD buflen, PDWORD returnlen)
{
    BOOL ret;
    WCHAR *volumenameW = NULL, *volumepathnameW;

    if (volumename && !(volumenameW = FILE_name_AtoW( volumename, TRUE ))) return FALSE;
    if (!(volumepathnameW = HeapAlloc( GetProcessHeap(), 0, buflen * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, volumenameW );
        return FALSE;
    }
    if ((ret = GetVolumePathNamesForVolumeNameW( volumenameW, volumepathnameW, buflen, returnlen )))
    {
        char *path = volumepathname;
        const WCHAR *pathW = volumepathnameW;

        while (*pathW)
        {
            int len = strlenW( pathW ) + 1;
            FILE_name_WtoA( pathW, len, path, buflen );
            buflen -= len;
            pathW += len;
            path += len;
        }
        path[0] = 0;
    }
    HeapFree( GetProcessHeap(), 0, volumenameW );
    HeapFree( GetProcessHeap(), 0, volumepathnameW );
    return ret;
}

static MOUNTMGR_MOUNT_POINTS *query_mount_points( HANDLE mgr, MOUNTMGR_MOUNT_POINT *input, DWORD insize )
{
    MOUNTMGR_MOUNT_POINTS *output;
    DWORD outsize = 1024;
    DWORD br;

    for (;;)
    {
        if (!(output = HeapAlloc( GetProcessHeap(), 0, outsize )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }
        if (DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_POINTS, input, insize, output, outsize, &br, NULL )) break;
        outsize = output->Size;
        HeapFree( GetProcessHeap(), 0, output );
        if (GetLastError() != ERROR_MORE_DATA) return NULL;
    }
    return output;
}
/***********************************************************************
 *           GetVolumePathNamesForVolumeNameW   (KERNEL32.@)
 */
BOOL WINAPI GetVolumePathNamesForVolumeNameW(LPCWSTR volumename, LPWSTR volumepathname, DWORD buflen, PDWORD returnlen)
{
    static const WCHAR dosdevicesW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\'};
    HANDLE mgr;
    DWORD len, size;
    MOUNTMGR_MOUNT_POINT *spec;
    MOUNTMGR_MOUNT_POINTS *link, *target = NULL;
    WCHAR *name, *path;
    BOOL ret = FALSE;
    UINT i, j;

    TRACE("%s, %p, %u, %p\n", debugstr_w(volumename), volumepathname, buflen, returnlen);

    if (!volumename || (len = strlenW( volumename )) != 49)
    {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }
    mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return FALSE;

    size = sizeof(*spec) + sizeof(WCHAR) * (len - 1); /* remove trailing backslash */
    if (!(spec = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) goto done;
    spec->SymbolicLinkNameOffset = sizeof(*spec);
    spec->SymbolicLinkNameLength = size - sizeof(*spec);
    name = (WCHAR *)((char *)spec + spec->SymbolicLinkNameOffset);
    memcpy( name, volumename, size - sizeof(*spec) );
    name[1] = '?'; /* map \\?\ to \??\ */

    target = query_mount_points( mgr, spec, size );
    HeapFree( GetProcessHeap(), 0, spec );
    if (!target)
    {
        goto done;
    }
    if (!target->NumberOfMountPoints)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        goto done;
    }
    len = 0;
    path = volumepathname;
    for (i = 0; i < target->NumberOfMountPoints; i++)
    {
        link = NULL;
        if (target->MountPoints[i].DeviceNameOffset)
        {
            const WCHAR *device = (const WCHAR *)((const char *)target + target->MountPoints[i].DeviceNameOffset);
            USHORT device_len = target->MountPoints[i].DeviceNameLength;

            size = sizeof(*spec) + device_len;
            if (!(spec = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) goto done;
            spec->DeviceNameOffset = sizeof(*spec);
            spec->DeviceNameLength = device_len;
            memcpy( (char *)spec + spec->DeviceNameOffset, device, device_len );

            link = query_mount_points( mgr, spec, size );
            HeapFree( GetProcessHeap(), 0, spec );
        }
        else if (target->MountPoints[i].UniqueIdOffset)
        {
            const WCHAR *id = (const WCHAR *)((const char *)target + target->MountPoints[i].UniqueIdOffset);
            USHORT id_len = target->MountPoints[i].UniqueIdLength;

            size = sizeof(*spec) + id_len;
            if (!(spec = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) goto done;
            spec->UniqueIdOffset = sizeof(*spec);
            spec->UniqueIdLength = id_len;
            memcpy( (char *)spec + spec->UniqueIdOffset, id, id_len );

            link = query_mount_points( mgr, spec, size );
            HeapFree( GetProcessHeap(), 0, spec );
        }
        if (!link) continue;
        for (j = 0; j < link->NumberOfMountPoints; j++)
        {
            const WCHAR *linkname;

            if (!link->MountPoints[j].SymbolicLinkNameOffset) continue;
            linkname = (const WCHAR *)((const char *)link + link->MountPoints[j].SymbolicLinkNameOffset);

            if (link->MountPoints[j].SymbolicLinkNameLength == sizeof(dosdevicesW) + 2 * sizeof(WCHAR) &&
                !memicmpW( linkname, dosdevicesW, sizeof(dosdevicesW) / sizeof(WCHAR) ))
            {
                len += 4;
                if (volumepathname && len < buflen)
                {
                    path[0] = linkname[sizeof(dosdevicesW) / sizeof(WCHAR)];
                    path[1] = ':';
                    path[2] = '\\';
                    path[3] = 0;
                    path += 4;
                }
            }
        }
        HeapFree( GetProcessHeap(), 0, link );
    }
    if (buflen <= len) SetLastError( ERROR_MORE_DATA );
    else if (volumepathname)
    {
        volumepathname[len] = 0;
        ret = TRUE;
    }
    if (returnlen) *returnlen = len + 1;

done:
    HeapFree( GetProcessHeap(), 0, target );
    CloseHandle( mgr );
    return ret;
}

/***********************************************************************
 *           FindFirstVolumeA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstVolumeA(LPSTR volume, DWORD len)
{
    WCHAR *buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    HANDLE handle = FindFirstVolumeW( buffer, len );

    if (handle != INVALID_HANDLE_VALUE)
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL ))
        {
            FindVolumeClose( handle );
            handle = INVALID_HANDLE_VALUE;
        }
    }
    HeapFree( GetProcessHeap(), 0, buffer );
    return handle;
}

/***********************************************************************
 *           FindFirstVolumeW   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstVolumeW( LPWSTR volume, DWORD len )
{
    DWORD size = 1024;
    DWORD br;
    HANDLE mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    for (;;)
    {
        MOUNTMGR_MOUNT_POINT input;
        MOUNTMGR_MOUNT_POINTS *output;

        if (!(output = HeapAlloc( GetProcessHeap(), 0, size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            break;
        }
        memset( &input, 0, sizeof(input) );

        if (!DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_POINTS, &input, sizeof(input),
                              output, size, &br, NULL ))
        {
            if (GetLastError() != ERROR_MORE_DATA) break;
            size = output->Size;
            HeapFree( GetProcessHeap(), 0, output );
            continue;
        }
        CloseHandle( mgr );
        /* abuse the Size field to store the current index */
        output->Size = 0;
        if (!FindNextVolumeW( output, volume, len ))
        {
            HeapFree( GetProcessHeap(), 0, output );
            return INVALID_HANDLE_VALUE;
        }
        return output;
    }
    CloseHandle( mgr );
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           FindNextVolumeA   (KERNEL32.@)
 */
BOOL WINAPI FindNextVolumeA( HANDLE handle, LPSTR volume, DWORD len )
{
    WCHAR *buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    BOOL ret;

    if ((ret = FindNextVolumeW( handle, buffer, len )))
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL )) ret = FALSE;
    }
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

/***********************************************************************
 *           FindNextVolumeW   (KERNEL32.@)
 */
BOOL WINAPI FindNextVolumeW( HANDLE handle, LPWSTR volume, DWORD len )
{
    MOUNTMGR_MOUNT_POINTS *data = handle;

    while (data->Size < data->NumberOfMountPoints)
    {
        static const WCHAR volumeW[] = {'\\','?','?','\\','V','o','l','u','m','e','{',};
        WCHAR *link = (WCHAR *)((char *)data + data->MountPoints[data->Size].SymbolicLinkNameOffset);
        DWORD size = data->MountPoints[data->Size].SymbolicLinkNameLength;
        data->Size++;
        /* skip non-volumes */
        if (size < sizeof(volumeW) || memcmp( link, volumeW, sizeof(volumeW) )) continue;
        if (size + sizeof(WCHAR) >= len * sizeof(WCHAR))
        {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
            return FALSE;
        }
        memcpy( volume, link, size );
        volume[1] = '\\';  /* map \??\ to \\?\ */
        volume[size / sizeof(WCHAR)] = '\\';  /* Windows appends a backslash */
        volume[size / sizeof(WCHAR) + 1] = 0;
        TRACE( "returning entry %u %s\n", data->Size - 1, debugstr_w(volume) );
        return TRUE;
    }
    SetLastError( ERROR_NO_MORE_FILES );
    return FALSE;
}

/***********************************************************************
 *           FindVolumeClose   (KERNEL32.@)
 */
BOOL WINAPI FindVolumeClose(HANDLE handle)
{
    return HeapFree( GetProcessHeap(), 0, handle );
}

/***********************************************************************
 *           FindFirstVolumeMountPointA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstVolumeMountPointA(LPCSTR root, LPSTR mount_point, DWORD len)
{
    FIXME("(%s, %p, %d), stub!\n", debugstr_a(root), mount_point, len);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           FindFirstVolumeMountPointW   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstVolumeMountPointW(LPCWSTR root, LPWSTR mount_point, DWORD len)
{
    FIXME("(%s, %p, %d), stub!\n", debugstr_w(root), mount_point, len);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           FindVolumeMountPointClose   (KERNEL32.@)
 */
BOOL WINAPI FindVolumeMountPointClose(HANDLE h)
{
    FIXME("(%p), stub!\n", h);
    return FALSE;
}

/***********************************************************************
 *           DeleteVolumeMountPointA   (KERNEL32.@)
 */
BOOL WINAPI DeleteVolumeMountPointA(LPCSTR mountpoint)
{
    FIXME("(%s), stub!\n", debugstr_a(mountpoint));
    return FALSE;
}

/***********************************************************************
 *           DeleteVolumeMountPointW (KERNEL32.@)
 */
BOOL WINAPI DeleteVolumeMountPointW(LPCWSTR mountpoint)
{
    FIXME("(%s), stub!\n", debugstr_w(mountpoint));
    return FALSE;
}
