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
#include "kernel_private.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(volume);

#define SUPERBLOCK_SIZE 2048

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
    FS_ISO9660
};

static const WCHAR drive_types[][8] =
{
    { 0 }, /* DRIVE_UNKNOWN */
    { 0 }, /* DRIVE_NO_ROOT_DIR */
    {'f','l','o','p','p','y',0}, /* DRIVE_REMOVABLE */
    {'h','d',0}, /* DRIVE_FIXED */
    {'n','e','t','w','o','r','k',0}, /* DRIVE_REMOTE */
    {'c','d','r','o','m',0}, /* DRIVE_CDROM */
    {'r','a','m','d','i','s','k',0} /* DRIVE_RAMDISK */
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


/* fetch the type of a drive from the registry */
static UINT get_registry_drive_type( const WCHAR *root )
{
    static const WCHAR drive_types_keyW[] = {'M','a','c','h','i','n','e','\\',
                                             'S','o','f','t','w','a','r','e','\\',
                                             'W','i','n','e','\\',
                                             'D','r','i','v','e','s',0 };
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE hkey;
    DWORD dummy;
    UINT ret = DRIVE_UNKNOWN;
    char tmp[32 + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    WCHAR driveW[] = {'A',':',0};

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, drive_types_keyW );
    /* @@ Wine registry key: HKLM\Software\Wine\Drives */
    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ) != STATUS_SUCCESS) return DRIVE_UNKNOWN;

    if (root) driveW[0] = root[0];
    else
    {
        WCHAR path[MAX_PATH];
        GetCurrentDirectoryW( MAX_PATH, path );
        driveW[0] = path[0];
    }

    RtlInitUnicodeString( &nameW, driveW );
    if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
    {
        unsigned int i;
        WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;

        for (i = 0; i < sizeof(drive_types)/sizeof(drive_types[0]); i++)
        {
            if (!strcmpiW( data, drive_types[i] ))
            {
                ret = i;
                break;
            }
        }
    }
    NtClose( hkey );
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
        !ReadFile( handle, buff, SUPERBLOCK_SIZE, &size, NULL ) ||
        size != SUPERBLOCK_SIZE)
        return FS_ERROR;

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
        reasonable = num_boot_sectors < 16 &&
                     num_fats < 16 &&
                     bytes_per_sector >= 512 && bytes_per_sector % 512 == 0 &&
                     sectors_per_cluster > 1;
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
 *           VOLUME_ReadCDSuperblock
 */
static enum fs_type VOLUME_ReadCDSuperblock( HANDLE handle, BYTE *buff )
{
    DWORD size, offs = VOLUME_FindCdRomDataBestVoldesc( handle );

    if (!offs) return FS_UNKNOWN;

    if (SetFilePointer( handle, offs, NULL, FILE_BEGIN ) != offs ||
        !ReadFile( handle, buff, SUPERBLOCK_SIZE, &size, NULL ) ||
        size != SUPERBLOCK_SIZE)
        return FS_ERROR;

    /* check for iso9660 present */
    if (!memcmp(&buff[1], "CD001", 5)) return FS_ISO9660;
    return FS_UNKNOWN;
}


/**************************************************************************
 *                              VOLUME_GetSuperblockLabel
 */
static void VOLUME_GetSuperblockLabel( enum fs_type type, const BYTE *superblock,
                                       WCHAR *label, DWORD len )
{
    const BYTE *label_ptr = NULL;
    DWORD label_len;

    switch(type)
    {
    case FS_ERROR:
    case FS_UNKNOWN:
        label_len = 0;
        break;
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
    }
    if (label_len) RtlMultiByteToUnicodeN( label, (len-1) * sizeof(WCHAR),
                                           &label_len, (LPCSTR)label_ptr, label_len );
    label_len /= sizeof(WCHAR);
    label[label_len] = 0;
    while (label_len && label[label_len-1] == ' ') label[--label_len] = 0;
}


/**************************************************************************
 *                              VOLUME_GetSuperblockSerial
 */
static DWORD VOLUME_GetSuperblockSerial( enum fs_type type, const BYTE *superblock )
{
    switch(type)
    {
    case FS_ERROR:
    case FS_UNKNOWN:
        break;
    case FS_FAT1216:
        return GETLONG( superblock, 0x27 );
    case FS_FAT32:
        return GETLONG( superblock, 0x33 );
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
            if (GetVersion() & 0x80000000)
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
    static const WCHAR ntfsW[] = {'N','T','F','S',0};
    static const WCHAR cdfsW[] = {'C','D','F','S',0};

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
        CDROM_TOC toc;
        DWORD br;

        /* check for audio CD */
        /* FIXME: we only check the first track for now */
        if (DeviceIoControl( handle, IOCTL_CDROM_READ_TOC, NULL, 0, &toc, sizeof(toc), &br, 0 ))
        {
            if (!(toc.TrackData[0].Control & 0x04))  /* audio track */
            {
                TRACE( "%s: found audio CD\n", debugstr_w(device) );
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
        CloseHandle( handle );
        TRACE( "%s: found fs type %d\n", debugstr_w(device), type );
        if (type == FS_ERROR) return FALSE;

        if (label && label_len) VOLUME_GetSuperblockLabel( type, superblock, label, label_len );
        if (serial) *serial = VOLUME_GetSuperblockSerial( type, superblock );
        goto fill_fs_info;
    }
    else TRACE( "cannot open device %s: err %d\n", debugstr_w(device), GetLastError() );

    /* we couldn't open the device, fallback to default strategy */

    switch(GetDriveTypeW( root ))
    {
    case DRIVE_UNKNOWN:
    case DRIVE_NO_ROOT_DIR:
        SetLastError( ERROR_NOT_READY );
        return FALSE;
    case DRIVE_REMOVABLE:
    case DRIVE_FIXED:
    case DRIVE_REMOTE:
    case DRIVE_RAMDISK:
        type = FS_UNKNOWN;
        break;
    case DRIVE_CDROM:
        type = FS_ISO9660;
        break;
    }

    if (label && label_len)
    {
        WCHAR labelW[] = {'A',':','\\','.','w','i','n','d','o','w','s','-','l','a','b','e','l',0};

        labelW[0] = device[4];
        handle = CreateFileW( labelW, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, 0, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            char buffer[256], *p;
            DWORD size;

            if (!ReadFile( handle, buffer, sizeof(buffer)-1, &size, NULL )) size = 0;
            CloseHandle( handle );
            p = buffer + size;
            while (p > buffer && (p[-1] == ' ' || p[-1] == '\r' || p[-1] == '\n')) p--;
            *p = 0;
            if (!MultiByteToWideChar( CP_UNIXCP, 0, buffer, -1, label, label_len ))
                label[label_len-1] = 0;
        }
        else label[0] = 0;
    }
    if (serial)
    {
        WCHAR serialW[] = {'A',':','\\','.','w','i','n','d','o','w','s','-','s','e','r','i','a','l',0};

        serialW[0] = device[4];
        handle = CreateFileW( serialW, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, 0, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            char buffer[32];
            DWORD size;

            if (!ReadFile( handle, buffer, sizeof(buffer)-1, &size, NULL )) size = 0;
            CloseHandle( handle );
            buffer[size] = 0;
            *serial = strtoul( buffer, NULL, 16 );
        }
        else *serial = 0;
    }

fill_fs_info:  /* now fill in the information that depends on the file system type */

    switch(type)
    {
    case FS_ISO9660:
        if (fsname) lstrcpynW( fsname, cdfsW, fsname_len );
        if (filename_len) *filename_len = 221;
        if (flags) *flags = FILE_READ_ONLY_VOLUME;
        break;
    case FS_FAT1216:
    case FS_FAT32:
        if (fsname) lstrcpynW( fsname, fatW, fsname_len );
        if (filename_len) *filename_len = 255;
        if (flags) *flags = FILE_CASE_PRESERVED_NAMES;  /* FIXME */
        break;
    default:
        if (fsname) lstrcpynW( fsname, ntfsW, fsname_len );
        if (filename_len) *filename_len = 255;
        if (flags) *flags = FILE_CASE_PRESERVED_NAMES;
        break;
    }
    return TRUE;
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
            handle = CreateFileW( labelW, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                  CREATE_ALWAYS, 0, 0 );
            if (handle != INVALID_HANDLE_VALUE)
            {
                char buffer[64];
                DWORD size;

                if (!WideCharToMultiByte( CP_UNIXCP, 0, label, -1, buffer, sizeof(buffer), NULL, NULL ))
                    buffer[sizeof(buffer)-1] = 0;
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
    BOOL ret = FALSE;
    static const WCHAR fmt[] =
        { '\\','\\','?','\\','V','o','l','u','m','e','{','%','0','2','x','}','\\',0 };

    TRACE("(%s, %p, %x)\n", debugstr_w(path), volume, size);

    if (!path || !path[0]) return FALSE;

    if (size >= sizeof(fmt) / sizeof(WCHAR))
    {
        /* FIXME: will break when we support volume mounts */
        sprintfW( volume, fmt, tolowerW( path[0] ) - 'a' );
        ret = TRUE;
    }
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
    static const WCHAR rootW[] = {'A',':','\\',0};
    static const WCHAR com0W[] = {'\\','?','?','\\','C','O','M','0',0};
    static const WCHAR com1W[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','C','O','M','1',0,0};
    static const WCHAR lpt1W[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','L','P','T','1',0,0};

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
        else if (devname[0] && devname[1] == ':' && !devname[2])
        {
            memcpy( name, devname, 3 * sizeof(WCHAR) );
        }
        else
        {
            SetLastError( ERROR_BAD_PATHNAME );
            return 0;
        }

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

        if (ret)
        {
            if (ret < bufsize) target[ret++] = 0;  /* add an extra null */
            for (p = target; *p; p++) if (*p == '/') *p = '\\';
        }

        return ret;
    }
    else  /* return a list of all devices */
    {
        WCHAR *p = target;
        int i;

        if (bufsize <= (sizeof(auxW)+sizeof(nulW)+sizeof(prnW))/sizeof(WCHAR))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }

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

        strcpyW( nt_buffer + 4, rootW );
        RtlInitUnicodeString( &nt_name, nt_buffer );

        for (i = 0; i < 26; i++)
        {
            nt_buffer[4] = 'a' + i;
            if (!wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, TRUE ))
            {
                RtlFreeAnsiString( &unix_name );
                if (p + 3 >= target + bufsize)
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    return 0;
                }
                *p++ = 'A' + i;
                *p++ = ':';
                *p++ = 0;
            }
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
    const char *config_dir = wine_get_config_dir();
    struct stat st;
    char *buffer, *dev;
    DWORD ret = 0;
    int i;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, strlen(config_dir) + sizeof("/dosdevices/a:") )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    strcpy( buffer, config_dir );
    strcat( buffer, "/dosdevices/a:" );
    dev = buffer + strlen(buffer) - 2;

    for (i = 0; i < 26; i++)
    {
        *dev = 'a' + i;
        if (!stat( buffer, &st )) ret |= (1 << i);
    }
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
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
            *buffer++ = 'a' + drive;
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
            *buffer++ = 'a' + drive;
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
    else if ((ret = get_registry_drive_type( root )) == DRIVE_UNKNOWN)
    {
        switch (info.DeviceType)
        {
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:  ret = DRIVE_CDROM; break;
        case FILE_DEVICE_VIRTUAL_DISK:        ret = DRIVE_RAMDISK; break;
        case FILE_DEVICE_NETWORK_FILE_SYSTEM: ret = DRIVE_REMOTE; break;
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            if (info.Characteristics & FILE_REMOTE_DEVICE) ret = DRIVE_REMOTE;
            else if (info.Characteristics & FILE_REMOVABLE_MEDIA) ret = DRIVE_REMOVABLE;
            else ret = DRIVE_FIXED;
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
    FIXME("(%s, %p, %d), stub!\n", debugstr_a(filename), volumepathname, buflen);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetVolumePathNameW   (KERNEL32.@)
 */
BOOL WINAPI GetVolumePathNameW(LPCWSTR filename, LPWSTR volumepathname, DWORD buflen)
{
    FIXME("(%s, %p, %d), stub!\n", debugstr_w(filename), volumepathname, buflen);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
