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
#include "ddk/wdm.h"
#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(volume);

#define BLOCK_SIZE 2048
#define SUPERBLOCK_SIZE BLOCK_SIZE

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


/***********************************************************************
 *           SetVolumeLabelW   (KERNEL32.@)
 */
BOOL WINAPI SetVolumeLabelW( LPCWSTR root, LPCWSTR label )
{
    WCHAR device[] = L"\\\\.\\A:";
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
        TRACE( "cannot open device %s: err %ld\n", debugstr_w(device), GetLastError() );
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
            WCHAR labelW[] = L"A:\\.windows-label";

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
    DWORD len = min(ARRAY_SIZE(volumeW), size );

    TRACE("(%s, %p, %lx)\n", debugstr_a(path), volume, size);

    if (!path || !(pathW = FILE_name_AtoW( path, TRUE )))
        return FALSE;

    if ((ret = GetVolumeNameForVolumeMountPointW( pathW, volumeW, len )))
        FILE_name_WtoA( volumeW, -1, volume, len );

    HeapFree( GetProcessHeap(), 0, pathW );
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
 *           GetVolumePathNameA   (KERNEL32.@)
 */
BOOL WINAPI GetVolumePathNameA(LPCSTR filename, LPSTR volumepathname, DWORD buflen)
{
    BOOL ret;
    WCHAR *filenameW = NULL, *volumeW = NULL;

    TRACE("(%s, %p, %ld)\n", debugstr_a(filename), volumepathname, buflen);

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
            int len = lstrlenW( pathW ) + 1;
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
 *           FindFirstVolumeMountPointA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstVolumeMountPointA(LPCSTR root, LPSTR mount_point, DWORD len)
{
    FIXME("(%s, %p, %ld), stub!\n", debugstr_a(root), mount_point, len);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           FindFirstVolumeMountPointW   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstVolumeMountPointW(LPCWSTR root, LPWSTR mount_point, DWORD len)
{
    FIXME("(%s, %p, %ld), stub!\n", debugstr_w(root), mount_point, len);
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
 *           SetVolumeMountPointA (KERNEL32.@)
 */
BOOL WINAPI SetVolumeMountPointA(LPCSTR path, LPCSTR volume)
{
    FIXME("(%s, %s), stub!\n", debugstr_a(path), debugstr_a(volume));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           SetVolumeMountPointW (KERNEL32.@)
 */
BOOL WINAPI SetVolumeMountPointW(LPCWSTR path, LPCWSTR volume)
{
    FIXME("(%s, %s), stub!\n", debugstr_w(path), debugstr_w(volume));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
