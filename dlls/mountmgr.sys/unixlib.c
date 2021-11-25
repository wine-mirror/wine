/*
 * MountMgr Unix interface
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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "unixlib.h"


static char *get_dosdevices_path( const char *dev )
{
    const char *home = getenv( "HOME" );
    const char *prefix = getenv( "WINEPREFIX" );
    size_t len = (prefix ? strlen(prefix) : strlen(home) + strlen("/.wine")) + sizeof("/dosdevices/") + strlen(dev);
    char *path = malloc( len );

    if (path)
    {
        if (prefix) strcpy( path, prefix );
        else
        {
            strcpy( path, home );
            strcat( path, "/.wine" );
        }
        strcat( path, "/dosdevices/" );
        strcat( path, dev );
    }
    return path;
}

static BOOL is_valid_device( struct stat *st )
{
#if defined(linux) || defined(__sun__)
    return S_ISBLK( st->st_mode );
#else
    /* disks are char devices on *BSD */
    return S_ISCHR( st->st_mode );
#endif
}

static void detect_devices( const char **paths, char *names, ULONG size )
{
    while (*paths)
    {
        char unix_path[32];
        unsigned int i = 0;

        for (;;)
        {
            int len = sprintf( unix_path, *paths, i++ );
            if (len + 2 > size) break;
            if (access( unix_path, F_OK ) != 0) break;
            strcpy( names, unix_path );
            names += len + 1;
            size -= len + 1;
        }
        paths++;
    }
    *names = 0;
}

/* find or create a DOS drive for the corresponding Unix device */
NTSTATUS add_drive( const char *device, enum device_type type, int *letter )
{
    char *path, *p;
    char in_use[26];
    struct stat dev_st, drive_st;
    int drive, first, last, avail = 0;

    if (stat( device, &dev_st ) == -1 || !is_valid_device( &dev_st )) return STATUS_NO_SUCH_DEVICE;

    if (!(path = get_dosdevices_path( "a::" ))) return STATUS_NO_MEMORY;
    p = path + strlen(path) - 3;

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
    free( path );
    return STATUS_OBJECT_NAME_COLLISION;

done:
    free( path );
    *letter = drive;
    return STATUS_SUCCESS;
}

NTSTATUS get_dosdev_symlink( const char *dev, char *buffer, ULONG size )
{
    char *path;
    int ret;

    if (!(path = get_dosdevices_path( dev ))) return STATUS_NO_MEMORY;

    ret = readlink( path, buffer, size );
    free( path );
    if (ret == -1) return STATUS_NO_SUCH_DEVICE;
    if (ret == size) return STATUS_BUFFER_TOO_SMALL;
    buffer[ret] = 0;
    return STATUS_SUCCESS;
}

NTSTATUS set_dosdev_symlink( const char *dev, const char *dest )
{
    char *path;
    NTSTATUS status = STATUS_SUCCESS;

    if (!(path = get_dosdevices_path( dev ))) return STATUS_NO_MEMORY;

    if (dest && dest[0])
    {
        unlink( path );
        if (symlink( dest, path ) == -1) status = STATUS_ACCESS_DENIED;
    }
    else unlink( path );

    free( path );
    return status;
}

NTSTATUS get_volume_dos_devices( const char *mount_point, unsigned int *dosdev )
{
    struct stat dev_st, drive_st;
    char *path;
    int i;

    if (stat( mount_point, &dev_st ) == -1) return STATUS_NO_SUCH_DEVICE;
    if (!(path = get_dosdevices_path( "a:" ))) return STATUS_NO_MEMORY;

    *dosdev = 0;
    for (i = 0; i < 26; i++)
    {
        path[strlen(path) - 2] = 'a' + i;
        if (stat( path, &drive_st ) != -1 && drive_st.st_rdev == dev_st.st_rdev) *dosdev |= 1 << i;
    }
    free( path );
    return STATUS_SUCCESS;
}

NTSTATUS read_volume_file( const char *volume, const char *file, void *buffer, ULONG *size )
{
    int ret, fd = -1;
    char *name = malloc( strlen(volume) + strlen(file) + 2 );

    sprintf( name, "%s/%s", volume, file );

    if (name[0] != '/')
    {
        char *path = get_dosdevices_path( name );
        if (path) fd = open( path, O_RDONLY );
        free( path );
    }
    else fd = open( name, O_RDONLY );

    free( name );
    if (fd == -1) return STATUS_NO_SUCH_FILE;
    ret = read( fd, buffer, *size );
    close( fd );
    if (ret == -1) return STATUS_NO_SUCH_FILE;
    *size = ret;
    return STATUS_SUCCESS;
}

BOOL match_unixdev( const char *device, ULONGLONG unix_dev )
{
    struct stat st;

    return !stat( device, &st ) && st.st_rdev == unix_dev;
}

NTSTATUS detect_serial_ports( char *names, ULONG size )
{
    static const char *paths[] =
    {
#ifdef linux
        "/dev/ttyS%u",
        "/dev/ttyUSB%u",
        "/dev/ttyACM%u",
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
        "/dev/cuau%u",
#elif defined(__DragonFly__)
        "/dev/cuaa%u",
#endif
        NULL
    };

    detect_devices( paths, names, size );
    return STATUS_SUCCESS;
}

NTSTATUS detect_parallel_ports( char *names, ULONG size )
{
    static const char *paths[] =
    {
#ifdef linux
        "/dev/lp%u",
#endif
        NULL
    };

    detect_devices( paths, names, size );
    return STATUS_SUCCESS;
}
