/*
 * DOS drives handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 *
 * Label & serial number read support.
 *  (c) 1999 Petr Tomasek <tomasek@etf.cuni.cz>
 *  (c) 2000 Andreas Mohr (changes)
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

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/winbase16.h"   /* for GetCurrentTask */
#include "winerror.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
#include "file.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosfs);
WINE_DECLARE_DEBUG_CHANNEL(file);

typedef struct
{
    char     *root;      /* root dir in Unix format without trailing / */
    char     *device;    /* raw device path */
    dev_t     dev;       /* unix device number */
    ino_t     ino;       /* unix inode number */
} DOSDRIVE;


#define MAX_DOS_DRIVES  26

static DOSDRIVE DOSDrives[MAX_DOS_DRIVES];

/* strdup on the process heap */
inline static char *heap_strdup( const char *str )
{
    INT len = strlen(str) + 1;
    LPSTR p = HeapAlloc( GetProcessHeap(), 0, len );
    if (p) memcpy( p, str, len );
    return p;
}

/***********************************************************************
 *           DRIVE_Init
 */
int DRIVE_Init(void)
{
    int i, len, symlink_count = 0, count = 0;
    WCHAR driveW[] = {'M','a','c','h','i','n','e','\\','S','o','f','t','w','a','r','e','\\',
                      'W','i','n','e','\\','W','i','n','e','\\',
                      'C','o','n','f','i','g','\\','D','r','i','v','e',' ','A',0};
    WCHAR path[MAX_PATHNAME_LEN];
    char tmp[MAX_PATHNAME_LEN*sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    struct stat drive_stat_buffer;
    WCHAR *p;
    DOSDRIVE *drive;
    HKEY hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    char *root;
    const char *config_dir = wine_get_config_dir();

    static const WCHAR PathW[] = {'P','a','t','h',0};
    static const WCHAR DeviceW[] = {'D','e','v','i','c','e',0};

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* get the root of the drives from symlinks */

    root = NULL;
    for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, drive++)
    {
        if (!root)
        {
            root = HeapAlloc( GetProcessHeap(), 0, strlen(config_dir) + sizeof("/dosdevices/a:") );
            strcpy( root, config_dir );
            strcat( root, "/dosdevices/a:" );
        }
        root[strlen(root)-2] = 'a' + i;
        if (stat( root, &drive_stat_buffer ))
        {
            if (!lstat( root, &drive_stat_buffer))
                MESSAGE("Could not stat %s (%s), ignoring drive %c:\n",
                        root, strerror(errno), 'a' + i);
            continue;
        }
        if (!S_ISDIR(drive_stat_buffer.st_mode))
        {
            MESSAGE("%s is not a directory, ignoring drive %c:\n", root, 'a' + i );
            continue;
        }
        drive->root     = root;
        drive->device   = NULL;
        drive->dev      = drive_stat_buffer.st_dev;
        drive->ino      = drive_stat_buffer.st_ino;
        root = NULL;
        symlink_count++;
    }
    if (root) HeapFree( GetProcessHeap(), 0, root );

    /* now get the parameters from the config file */

    for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, drive++)
    {
        RtlInitUnicodeString( &nameW, driveW );
        nameW.Buffer[(nameW.Length / sizeof(WCHAR)) - 1] = 'A' + i;
        if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ) != STATUS_SUCCESS) continue;

        /* Get the root path */
        if (!symlink_count)
        {
            RtlInitUnicodeString( &nameW, PathW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
            {
                WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                ExpandEnvironmentStringsW( data, path, sizeof(path)/sizeof(WCHAR) );

                p = path + strlenW(path) - 1;
                while ((p > path) && (*p == '/')) *p-- = '\0';

                if (path[0] == '/')
                {
                    len = WideCharToMultiByte(CP_UNIXCP, 0, path, -1, NULL, 0, NULL, NULL);
                    drive->root = HeapAlloc(GetProcessHeap(), 0, len);
                    WideCharToMultiByte(CP_UNIXCP, 0, path, -1, drive->root, len, NULL, NULL);
                }
                else
                {
                    /* relative paths are relative to config dir */
                    const char *config = wine_get_config_dir();
                    len = strlen(config);
                    len += WideCharToMultiByte(CP_UNIXCP, 0, path, -1, NULL, 0, NULL, NULL) + 2;
                    drive->root = HeapAlloc( GetProcessHeap(), 0, len );
                    len -= sprintf( drive->root, "%s/", config );
                    WideCharToMultiByte(CP_UNIXCP, 0, path, -1,
                                        drive->root + strlen(drive->root), len, NULL, NULL);
                }

                if (stat( drive->root, &drive_stat_buffer ))
                {
                    MESSAGE("Could not stat %s (%s), ignoring drive %c:\n",
                            drive->root, strerror(errno), 'A' + i);
                    HeapFree( GetProcessHeap(), 0, drive->root );
                    drive->root = NULL;
                    goto next;
                }
                if (!S_ISDIR(drive_stat_buffer.st_mode))
                {
                    MESSAGE("%s is not a directory, ignoring drive %c:\n",
                            drive->root, 'A' + i );
                    HeapFree( GetProcessHeap(), 0, drive->root );
                    drive->root = NULL;
                    goto next;
                }

                drive->device   = NULL;
                drive->dev      = drive_stat_buffer.st_dev;
                drive->ino      = drive_stat_buffer.st_ino;
            }
        }

        if (drive->root)
        {
            /* Get the device */
            RtlInitUnicodeString( &nameW, DeviceW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
            {
                WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                len = WideCharToMultiByte(CP_UNIXCP, 0, data, -1, NULL, 0, NULL, NULL);
                drive->device = HeapAlloc(GetProcessHeap(), 0, len);
                WideCharToMultiByte(CP_UNIXCP, 0, data, -1, drive->device, len, NULL, NULL);
            }

            count++;
            TRACE("Drive %c: path=%s dev=%x ino=%x\n",
                  'A' + i, drive->root, (int)drive->dev, (int)drive->ino );
        }

    next:
        NtClose( hkey );
    }
    return 1;
}


/***********************************************************************
 *           DRIVE_IsValid
 */
int DRIVE_IsValid( int drive )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES)) return 0;
    return (DOSDrives[drive].root != NULL);
}


/***********************************************************************
 *           DRIVE_FindDriveRoot
 *
 * Find a drive for which the root matches the beginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, path is modified
 * to point to the beginning of the DOS path.
 *
 * Note: path must be in the encoding of the underlying Unix file system.
 */
int DRIVE_FindDriveRoot( const char **path )
{
    /* Starting with the full path, check if the device and inode match any of
     * the wine 'drives'. If not then remove the last path component and try
     * again. If the last component was a '..' then skip a normal component
     * since it's a directory that's ascended back out of.
     */
    int drive, level, len;
    char buffer[MAX_PATHNAME_LEN];
    char *p;
    struct stat st;

    strcpy( buffer, *path );
    for (p = buffer; *p; p++) if (*p == '\\') *p = '/';
    len = p - buffer;

    /* strip off trailing slashes */
    while (len > 1 && buffer[len - 1] == '/') buffer[--len] = 0;

    for (;;)
    {
        /* Find the drive */
        if (stat( buffer, &st ) == 0 && S_ISDIR( st.st_mode ))
        {
            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
               if (!DOSDrives[drive].root) continue;

               if ((DOSDrives[drive].dev == st.st_dev) &&
                   (DOSDrives[drive].ino == st.st_ino))
               {
                   if (len == 1) len = 0;  /* preserve root slash in returned path */
                   TRACE( "%s -> drive %c:, root='%s', name='%s'\n",
                       *path, 'A' + drive, buffer, *path + len);
                   *path += len;
                   if (!**path) *path = "\\";
                   return drive;
               }
            }
        }
        if (len <= 1) return -1;  /* reached root */

        level = 0;
        while (level < 1)
        {
            /* find start of the last path component */
            while (len > 1 && buffer[len - 1] != '/') len--;
            if (!buffer[len]) break;  /* empty component -> reached root */
            /* does removing it take us up a level? */
            if (strcmp( buffer + len, "." ) != 0)
                level += strcmp( buffer + len, ".." ) ? 1 : -1;
            buffer[len] = 0;
            /* strip off trailing slashes */
            while (len > 1 && buffer[len - 1] == '/') buffer[--len] = 0;
        }
    }
}


/***********************************************************************
 *           DRIVE_FindDriveRootW
 *
 * Unicode version of DRIVE_FindDriveRoot.
 */
int DRIVE_FindDriveRootW( LPCWSTR *path )
{
    int drive, level, len;
    WCHAR buffer[MAX_PATHNAME_LEN];
    WCHAR *p;
    struct stat st;

    strcpyW( buffer, *path );
    for (p = buffer; *p; p++) if (*p == '\\') *p = '/';
    len = p - buffer;

    /* strip off trailing slashes */
    while (len > 1 && buffer[len - 1] == '/') buffer[--len] = 0;

    for (;;)
    {
        char buffA[MAX_PATHNAME_LEN];

        WideCharToMultiByte( CP_UNIXCP, 0, buffer, -1, buffA, sizeof(buffA), NULL, NULL );
        if (stat( buffA, &st ) == 0 && S_ISDIR( st.st_mode ))
        {
            /* Find the drive */
            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
                if (!DOSDrives[drive].root) continue;

                if ((DOSDrives[drive].dev == st.st_dev) &&
                    (DOSDrives[drive].ino == st.st_ino))
                {
                    static const WCHAR rootW[] = {'\\',0};

                    if (len == 1) len = 0;  /* preserve root slash in returned path */
                    TRACE( "%s -> drive %c:, root=%s, name=%s\n",
                           debugstr_w(*path), 'A' + drive, debugstr_w(buffer), debugstr_w(*path + len));
                    *path += len;
                    if (!**path) *path = rootW;
                    return drive;
                }
            }
        }
        if (len <= 1) return -1;  /* reached root */

        level = 0;
        while (level < 1)
        {
            static const WCHAR dotW[] = {'.',0};
            static const WCHAR dotdotW[] = {'.','.',0};

            /* find start of the last path component */
            while (len > 1 && buffer[len - 1] != '/') len--;
            if (!buffer[len]) break;  /* empty component -> reached root */
            /* does removing it take us up a level? */
            if (strcmpW( buffer + len, dotW ) != 0)
                level += strcmpW( buffer + len, dotdotW ) ? 1 : -1;
            buffer[len] = 0;
            /* strip off trailing slashes */
            while (len > 1 && buffer[len - 1] == '/') buffer[--len] = 0;
        }
    }
}


/***********************************************************************
 *           DRIVE_GetRoot
 */
const char * DRIVE_GetRoot( int drive )
{
    if (!DRIVE_IsValid( drive )) return NULL;
    return DOSDrives[drive].root;
}


/***********************************************************************
 *           DRIVE_GetDevice
 */
const char * DRIVE_GetDevice( int drive )
{
    return (DRIVE_IsValid( drive )) ? DOSDrives[drive].device : NULL;
}
