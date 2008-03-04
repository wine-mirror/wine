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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "dbt.h"

#include "wine/library.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

struct dos_drive
{
    struct list entry;
    char *udi;
    int   drive;
};

static struct list drives_list = LIST_INIT(drives_list);

static char *get_dosdevices_path(void)
{
    const char *config_dir = wine_get_config_dir();
    size_t len = strlen(config_dir) + sizeof("/dosdevices/a::");
    char *path = HeapAlloc( GetProcessHeap(), 0, len );
    if (path)
    {
        strcpy( path, config_dir );
        strcat( path, "/dosdevices/a::" );
    }
    return path;
}

/* send notification about a change to a given drive */
static void send_notify( int drive, int code )
{
    DWORD_PTR result;
    DEV_BROADCAST_VOLUME info;

    info.dbcv_size       = sizeof(info);
    info.dbcv_devicetype = DBT_DEVTYP_VOLUME;
    info.dbcv_reserved   = 0;
    info.dbcv_unitmask   = 1 << drive;
    info.dbcv_flags      = DBTF_MEDIA;
    result = BroadcastSystemMessageW( BSF_FORCEIFHUNG|BSF_QUERY, NULL,
                                      WM_DEVICECHANGE, code, (LPARAM)&info );
}

static inline int is_valid_device( struct stat *st )
{
#if defined(linux) || defined(__sun__)
    return S_ISBLK( st->st_mode );
#else
    /* disks are char devices on *BSD */
    return S_ISCHR( st->st_mode );
#endif
}

/* find or create a DOS drive for the corresponding device */
static int add_drive( const char *device, const char *type )
{
    char *path, *p;
    char in_use[26];
    struct stat dev_st, drive_st;
    int drive, first, last, avail = 0;

    if (stat( device, &dev_st ) == -1 || !is_valid_device( &dev_st )) return -1;

    if (!(path = get_dosdevices_path())) return -1;
    p = path + strlen(path) - 3;

    memset( in_use, 0, sizeof(in_use) );

    first = 2;
    last = 26;
    if (type && !strcmp( type, "floppy" ))
    {
        first = 0;
        last = 2;
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

static BOOL set_mount_point( struct dos_drive *drive, const char *mount_point )
{
    char *path, *p;
    struct stat path_st, mnt_st;
    BOOL modified = FALSE;

    if (drive->drive == -1) return FALSE;
    if (!(path = get_dosdevices_path())) return FALSE;
    p = path + strlen(path) - 3;
    *p = 'a' + drive->drive;
    p[2] = 0;

    if (mount_point[0])
    {
        /* try to avoid unlinking if already set correctly */
        if (stat( path, &path_st ) == -1 || stat( mount_point, &mnt_st ) == -1 ||
            path_st.st_dev != mnt_st.st_dev || path_st.st_ino != mnt_st.st_ino)
        {
            unlink( path );
            symlink( mount_point, path );
            modified = TRUE;
        }
    }
    else
    {
        if (unlink( path ) != -1) modified = TRUE;
    }

    HeapFree( GetProcessHeap(), 0, path );
    return modified;
}

BOOL add_dos_device( const char *udi, const char *device,
                     const char *mount_point, const char *type )
{
    struct dos_drive *drive;

    /* first check if it already exists */
    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        if (!strcmp( udi, drive->udi )) goto found;
    }

    if (!(drive = HeapAlloc( GetProcessHeap(), 0, sizeof(*drive) ))) return FALSE;
    if (!(drive->udi = HeapAlloc( GetProcessHeap(), 0, strlen(udi)+1 )))
    {
        HeapFree( GetProcessHeap(), 0, drive );
        return FALSE;
    }
    strcpy( drive->udi, udi );
    list_add_tail( &drives_list, &drive->entry );

found:
    drive->drive = add_drive( device, type );
    if (drive->drive != -1)
    {
        HKEY hkey;

        set_mount_point( drive, mount_point );

        TRACE( "added device %c: udi %s for %s on %s type %s\n",
                    'a' + drive->drive, wine_dbgstr_a(udi), wine_dbgstr_a(device),
                    wine_dbgstr_a(mount_point), wine_dbgstr_a(type) );

        /* hack: force the drive type in the registry */
        if (!RegCreateKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Drives", &hkey ))
        {
            char name[3] = "a:";
            name[0] += drive->drive;
            if (!type || strcmp( type, "cdrom" )) type = "floppy";  /* FIXME: default to floppy */
            RegSetValueExA( hkey, name, 0, REG_SZ, (const BYTE *)type, strlen(type) + 1 );
            RegCloseKey( hkey );
        }

        send_notify( drive->drive, DBT_DEVICEARRIVAL );
    }
    return TRUE;
}

BOOL remove_dos_device( const char *udi )
{
    HKEY hkey;
    struct dos_drive *drive;

    LIST_FOR_EACH_ENTRY( drive, &drives_list, struct dos_drive, entry )
    {
        if (strcmp( udi, drive->udi )) continue;

        if (drive->drive != -1)
        {
            BOOL modified = set_mount_point( drive, "" );

            /* clear the registry key too */
            if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Drives", &hkey ))
            {
                char name[3] = "a:";
                name[0] += drive->drive;
                RegDeleteValueA( hkey, name );
                RegCloseKey( hkey );
            }

            if (modified) send_notify( drive->drive, DBT_DEVICEREMOVECOMPLETE );
        }

        list_remove( &drive->entry );
        HeapFree( GetProcessHeap(), 0, drive->udi );
        HeapFree( GetProcessHeap(), 0, drive );
        return TRUE;
    }
    return FALSE;
}
