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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "winternl.h"
#include "ntstatus.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
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


/******************************************************************
 *		VOLUME_FindCdRomDataBestVoldesc
 */
static DWORD VOLUME_FindCdRomDataBestVoldesc( HANDLE handle )
{
    BYTE cur_vd_type, max_vd_type = 0;
    BYTE buffer[16];
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

    if (buff[0] == 0xE9 || (buff[0] == 0xEB && buff[2] == 0x90))
    {
        /* guess which type of FAT we have */
        unsigned int sz, nsect, nclust;
        sz = GETWORD(buff, 0x16);
        if (!sz) sz = GETLONG(buff, 0x24);
        nsect = GETWORD(buff, 0x13);
        if (!nsect) nsect = GETLONG(buff, 0x20);
        nsect -= GETWORD(buff, 0x0e) + buff[0x10] * sz +
            (GETWORD(buff, 0x11) * 32 + (GETWORD(buff, 0x0b) - 1)) / GETWORD(buff, 0x0b);
        nclust = nsect / buff[0x0d];

        if (nclust < 65525)
        {
            if (buff[0x26] == 0x29 && !memcmp(buff+0x36, "FAT", 3))
            {
                /* FIXME: do really all FAT have their name beginning with
                 * "FAT" ? (At least FAT12, FAT16 and FAT32 have :)
                 */
                return FS_FAT1216;
            }
        }
        else if (!memcmp(buff+0x52, "FAT", 3)) return FS_FAT32;
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
                int i;

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
                                           &label_len, label_ptr, label_len );
    label_len /= sizeof(WCHAR);
    label[label_len] = 0;
    while (label_len && label[label_len-1] == ' ') label[--label_len] = 0;
}


/**************************************************************************
 *                              VOLUME_SetSuperblockLabel
 */
static BOOL VOLUME_SetSuperblockLabel( enum fs_type type, HANDLE handle, const WCHAR *label )
{
    BYTE label_data[11];
    DWORD offset, len;

    switch(type)
    {
    case FS_FAT1216:
        offset = 0x2b;
        break;
    case FS_FAT32:
        offset = 0x47;
        break;
    default:
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }
    RtlUnicodeToMultiByteN( label_data, sizeof(label_data), &len,
                            label, strlenW(label) * sizeof(WCHAR) );
    if (len < sizeof(label_data))
        memset( label_data + len, ' ', sizeof(label_data) - len );

    return (SetFilePointer( handle, offset, NULL, FILE_BEGIN ) == offset &&
            WriteFile( handle, label_data, sizeof(label_data), &len, NULL ));
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
    else
    {
        TRACE( "cannot open device %s: err %ld\n", debugstr_w(device), GetLastError() );
        if (GetLastError() != ERROR_ACCESS_DENIED) return FALSE;
    }

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
    default:  /* default to FAT file system (FIXME) */
        if (fsname) lstrcpynW( fsname, fatW, fsname_len );
        if (filename_len) *filename_len = 255;
        if (flags) *flags = FILE_CASE_PRESERVED_NAMES;  /* FIXME */
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
    UNICODE_STRING rootW;
    LPWSTR labelW, fsnameW;
    BOOL ret;

    if (root) RtlCreateUnicodeStringFromAsciiz(&rootW, root);
    else rootW.Buffer = NULL;
    labelW = label ? HeapAlloc(GetProcessHeap(), 0, label_len * sizeof(WCHAR)) : NULL;
    fsnameW = fsname ? HeapAlloc(GetProcessHeap(), 0, fsname_len * sizeof(WCHAR)) : NULL;

    if ((ret = GetVolumeInformationW(rootW.Buffer, labelW, label_len, serial,
                                    filename_len, flags, fsnameW, fsname_len)))
    {
        if (label) WideCharToMultiByte(CP_ACP, 0, labelW, -1, label, label_len, NULL, NULL);
        if (fsname) WideCharToMultiByte(CP_ACP, 0, fsnameW, -1, fsname, fsname_len, NULL, NULL);
    }

    RtlFreeUnicodeString(&rootW);
    if (labelW) HeapFree( GetProcessHeap(), 0, labelW );
    if (fsnameW) HeapFree( GetProcessHeap(), 0, fsnameW );
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

    handle = CreateFileW( device, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE)
    {
        /* try read-only */
        handle = CreateFileW( device, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            /* device can be read but not written, return error */
            CloseHandle( handle );
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }
    }

    if (handle != INVALID_HANDLE_VALUE)
    {
        BYTE superblock[SUPERBLOCK_SIZE];
        BOOL ret;

        type = VOLUME_ReadFATSuperblock( handle, superblock );
        ret = VOLUME_SetSuperblockLabel( type, handle, label );
        CloseHandle( handle );
        return ret;
    }
    else
    {
        TRACE( "cannot open device %s: err %ld\n", debugstr_w(device), GetLastError() );
        if (GetLastError() != ERROR_ACCESS_DENIED) return FALSE;
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
    UNICODE_STRING rootW, volnameW;
    BOOL ret;

    if (root) RtlCreateUnicodeStringFromAsciiz(&rootW, root);
    else rootW.Buffer = NULL;
    if (volname) RtlCreateUnicodeStringFromAsciiz(&volnameW, volname);
    else volnameW.Buffer = NULL;

    ret = SetVolumeLabelW( rootW.Buffer, volnameW.Buffer );

    RtlFreeUnicodeString(&rootW);
    RtlFreeUnicodeString(&volnameW);
    return ret;
}


/***********************************************************************
 *           GetVolumeNameForVolumeMountPointW   (KERNEL32.@)
 */
BOOL WINAPI GetVolumeNameForVolumeMountPointW(LPCWSTR str, LPWSTR dst, DWORD size)
{
    FIXME("(%s, %p, %lx): stub\n", debugstr_w(str), dst, size);
    return 0;
}
