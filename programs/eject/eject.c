/*
 * Eject CDs
 *
 * Copyright 2005 Alexandre Julliard for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winioctl.h>
#include <ntddstor.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(eject);

/* options */
static BOOL unmount_only;
static BOOL eject_all;

/* wrapper for GetDriveTypeW */
static DWORD get_drive_type( WCHAR drive )
{
    static const WCHAR rootW[] = {'a',':','\\',0};
    WCHAR path[16];

    memcpy( path, rootW, sizeof(rootW) );
    path[0] = drive;
    return GetDriveTypeW( path );
}

static BOOL eject_cd( WCHAR drive )
{
    static const WCHAR deviceW[] = {'\\','\\','.','\\','a',':',0};
    PREVENT_MEDIA_REMOVAL removal;
    WCHAR buffer[16];
    HANDLE handle;
    DWORD result;

    if (get_drive_type( drive ) != DRIVE_CDROM)
    {
        WINE_MESSAGE( "Drive %c: is not a CD or is not mounted\n", (char)drive );
        return FALSE;
    }

    memcpy( buffer, deviceW, sizeof(deviceW) );
    buffer[4] = drive;
    handle = CreateFileW( buffer, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE)
    {
        WINE_MESSAGE( "Cannot open device for drive %c:\n", (char)drive );
        return FALSE;
    }

    WINE_TRACE( "ejecting %c:\n", (char)drive );

    if (!DeviceIoControl( handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &result, NULL ))
        WINE_WARN( "FSCTL_DISMOUNT_VOLUME failed with err %d\n", GetLastError() );

    removal.PreventMediaRemoval = FALSE;
    if (!DeviceIoControl( handle, IOCTL_STORAGE_MEDIA_REMOVAL, &removal, sizeof(removal), NULL, 0, &result, NULL ))
        WINE_WARN( "IOCTL_STORAGE_MEDIA_REMOVAL failed with err %d\n", GetLastError() );

    if (!unmount_only)
    {
        if (!DeviceIoControl( handle, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &result, NULL ))
            WINE_WARN( "IOCTL_STORAGE_EJECT_MEDIA failed with err %d\n", GetLastError() );
    }

    CloseHandle( handle );
    return TRUE;
}

/* find the CD drive, and die if we find more than one */
static WCHAR find_cd_drive(void)
{
    WCHAR ret = 0, drive;

    for (drive = 'c'; drive <= 'z'; drive++)
    {
        if (get_drive_type( drive ) != DRIVE_CDROM) continue;
        if (ret)
        {
            WINE_MESSAGE( "Multiple CD drives found (%c: and %c:), you need to specify the one you want.\n",
                          (char)ret, (char)drive );
            exit(1);
        }
        ret = drive;
    }
    return ret;
}

static void usage(void)
{
    WINE_MESSAGE( "Usage: eject [-u] [-a] [-h] [x:]...\n" );
    WINE_MESSAGE( "    -a  Eject all the CD drives we find\n" );
    WINE_MESSAGE( "    -h  Display this help message\n" );
    WINE_MESSAGE( "    -u  Unmount only, don't eject the CD\n" );
    WINE_MESSAGE( "    x:  Eject drive x:\n" );
    exit(1);
}

static void parse_options( int *argc, char *argv[] )
{
    int i;
    char *opt;

    for (i = 1; i < *argc; i++)
    {
        if (argv[i][0] != '-')
        {
            /* check for valid drive argument */
            if (strlen(argv[i]) != 2 || argv[i][1] != ':') usage();
            continue;
        }
        for (opt = argv[i] + 1; *opt; opt++) switch(*opt)
        {
        case 'a': eject_all = TRUE; break;
        case 'u': unmount_only = TRUE; break;
        case 'h': usage(); break;
        default:
            WINE_MESSAGE( "Unknown option -%c\n", *opt );
            usage();
        }
        memmove( argv + i, argv + i + 1, (*argc - i) * sizeof(*argv) );
        (*argc)--;
        i--;
    }
}

int __cdecl main( int argc, char *argv[] )
{
    parse_options( &argc, argv );

    if (eject_all)
    {
        WCHAR drive;

        for (drive = 'c'; drive <= 'z'; drive++)
        {
            if (get_drive_type( drive ) != DRIVE_CDROM) continue;
            if (!eject_cd( drive )) exit(1);
        }
    }
    else if (argc > 1)
    {
        int i;

        for (i = 1; i < argc; i++)
            if (!eject_cd( argv[i][0] )) exit(1);
    }
    else
    {
        WCHAR drive = find_cd_drive();

        if (!drive)
        {
            WINE_MESSAGE( "No CD drive found\n" );
            exit(1);
        }
        if (!eject_cd( drive )) exit(1);
    }
    exit(0);
}
