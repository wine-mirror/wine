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
#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
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
    LPWSTR    dos_cwd;   /* cwd in DOS format without leading or trailing \ */
    char     *unix_cwd;  /* cwd in Unix format without leading or trailing / */
    char     *device;    /* raw device path */
    UINT      type;      /* drive type */
    UINT      flags;     /* drive flags */
    dev_t     dev;       /* unix device number */
    ino_t     ino;       /* unix inode number */
} DOSDRIVE;


static const WCHAR DRIVE_Types[][8] =
{
    { 0 }, /* DRIVE_UNKNOWN */
    { 0 }, /* DRIVE_NO_ROOT_DIR */
    {'f','l','o','p','p','y',0}, /* DRIVE_REMOVABLE */
    {'h','d',0}, /* DRIVE_FIXED */
    {'n','e','t','w','o','r','k',0}, /* DRIVE_REMOTE */
    {'c','d','r','o','m',0}, /* DRIVE_CDROM */
    {'r','a','m','d','i','s','k',0} /* DRIVE_RAMDISK */
};

#define MAX_DOS_DRIVES  26

static DOSDRIVE DOSDrives[MAX_DOS_DRIVES];
static int DRIVE_CurDrive = -1;

static HTASK16 DRIVE_LastTask = 0;

/* strdup on the process heap */
inline static char *heap_strdup( const char *str )
{
    INT len = strlen(str) + 1;
    LPSTR p = HeapAlloc( GetProcessHeap(), 0, len );
    if (p) memcpy( p, str, len );
    return p;
}

#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

extern void CDROM_InitRegistry(int dev);

/***********************************************************************
 *           DRIVE_GetDriveType
 */
static inline UINT DRIVE_GetDriveType( INT drive, LPCWSTR value )
{
    int i;

    for (i = 0; i < sizeof(DRIVE_Types)/sizeof(DRIVE_Types[0]); i++)
    {
        if (!strcmpiW( value, DRIVE_Types[i] )) return i;
    }
    MESSAGE("Drive %c: unknown drive type %s, defaulting to 'hd'.\n",
            'A' + drive, debugstr_w(value) );
    return DRIVE_FIXED;
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
    WCHAR drive_env[] = {'=','A',':',0};
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
    static const WCHAR TypeW[] = {'T','y','p','e',0};
    static const WCHAR DeviceW[] = {'D','e','v','i','c','e',0};
    static const WCHAR FailReadOnlyW[] = {'F','a','i','l','R','e','a','d','O','n','l','y',0};

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
        drive->dos_cwd  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(drive->dos_cwd[0]));
        drive->unix_cwd = heap_strdup( "" );
        drive->device   = NULL;
        drive->flags    = 0;
        drive->dev      = drive_stat_buffer.st_dev;
        drive->ino      = drive_stat_buffer.st_ino;
        drive->type     = DRIVE_FIXED;
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

                drive->dos_cwd  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(drive->dos_cwd[0]));
                drive->unix_cwd = heap_strdup( "" );
                drive->device   = NULL;
                drive->flags    = 0;
                drive->dev      = drive_stat_buffer.st_dev;
                drive->ino      = drive_stat_buffer.st_ino;
                drive->type     = DRIVE_FIXED;
            }
        }

        if (drive->root)
        {
            /* Get the drive type */
            RtlInitUnicodeString( &nameW, TypeW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
            {
                WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                drive->type = DRIVE_GetDriveType( i, data );
            }

            /* Get the device */
            RtlInitUnicodeString( &nameW, DeviceW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
            {
                WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                len = WideCharToMultiByte(CP_UNIXCP, 0, data, -1, NULL, 0, NULL, NULL);
                drive->device = HeapAlloc(GetProcessHeap(), 0, len);
                WideCharToMultiByte(CP_UNIXCP, 0, data, -1, drive->device, len, NULL, NULL);

                if (drive->type == DRIVE_CDROM)
                {
                    int cd_fd;
                    if ((cd_fd = open(drive->device, O_RDONLY|O_NONBLOCK)) != -1)
                    {
                        CDROM_InitRegistry(cd_fd);
                        close(cd_fd);
                    }
                }
            }

            /* Get the FailReadOnly flag */
            RtlInitUnicodeString( &nameW, FailReadOnlyW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
            {
                WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                if (IS_OPTION_TRUE(data[0])) drive->flags |= DRIVE_FAIL_READ_ONLY;
            }

            /* Make the first hard disk the current drive */
            if ((DRIVE_CurDrive == -1) && (drive->type == DRIVE_FIXED))
                DRIVE_CurDrive = i;

            count++;
            TRACE("Drive %c: path=%s type=%s flags=%08x dev=%x ino=%x\n",
                  'A' + i, drive->root, debugstr_w(DRIVE_Types[drive->type]),
                  drive->flags, (int)drive->dev, (int)drive->ino );
        }

    next:
        NtClose( hkey );
    }

    if (!count && !symlink_count)
    {
        MESSAGE("Warning: no valid DOS drive found, check your configuration file.\n" );
        /* Create a C drive pointing to Unix root dir */
        DOSDrives[2].root     = heap_strdup( "/" );
        DOSDrives[2].dos_cwd  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DOSDrives[2].dos_cwd[0]));
        DOSDrives[2].unix_cwd = heap_strdup( "" );
        DOSDrives[2].type     = DRIVE_FIXED;
        DOSDrives[2].device   = NULL;
        DOSDrives[2].flags    = 0;
        DRIVE_CurDrive = 2;
    }

    /* Make sure the current drive is valid */
    if (DRIVE_CurDrive == -1)
    {
        for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, drive++)
        {
            if (drive->root)
            {
                DRIVE_CurDrive = i;
                break;
            }
        }
    }

    /* get current working directory info for all drives */
    for (i = 0; i < MAX_DOS_DRIVES; i++, drive_env[1]++)
    {
        if (!GetEnvironmentVariableW(drive_env, path, MAX_PATHNAME_LEN)) continue;
        /* sanity check */
        if (toupperW(path[0]) != drive_env[1] || path[1] != ':') continue;
        DRIVE_Chdir( i, path + 2 );
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
 *           DRIVE_GetCurrentDrive
 */
int DRIVE_GetCurrentDrive(void)
{
    TDB *pTask = GlobalLock16(GetCurrentTask());
    if (pTask && (pTask->curdrive & 0x80)) return pTask->curdrive & ~0x80;
    return DRIVE_CurDrive;
}


/***********************************************************************
 *           DRIVE_SetCurrentDrive
 */
int DRIVE_SetCurrentDrive( int drive )
{
    TDB *pTask = GlobalLock16(GetCurrentTask());
    if (!DRIVE_IsValid( drive ))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }
    TRACE("%c:\n", 'A' + drive );
    DRIVE_CurDrive = drive;
    if (pTask) pTask->curdrive = drive | 0x80;
    return 1;
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
 *           DRIVE_GetDosCwd
 */
LPCWSTR DRIVE_GetDosCwd( int drive )
{
    TDB *pTask = GlobalLock16(GetCurrentTask());
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        static const WCHAR rootW[] = {'\\',0};
        WCHAR curdirW[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, pTask->curdir, -1, curdirW, MAX_PATH);
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, curdirW )) DRIVE_Chdir( drive, rootW );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].dos_cwd;
}


/***********************************************************************
 *           DRIVE_GetUnixCwd
 */
const char * DRIVE_GetUnixCwd( int drive )
{
    TDB *pTask = GlobalLock16(GetCurrentTask());
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        static const WCHAR rootW[] = {'\\',0};
        WCHAR curdirW[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, pTask->curdir, -1, curdirW, MAX_PATH);
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, curdirW )) DRIVE_Chdir( drive, rootW );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].unix_cwd;
}


/***********************************************************************
 *           DRIVE_GetDevice
 */
const char * DRIVE_GetDevice( int drive )
{
    return (DRIVE_IsValid( drive )) ? DOSDrives[drive].device : NULL;
}

/***********************************************************************
 *           DRIVE_GetType
 */
static UINT DRIVE_GetType( int drive )
{
    if (!DRIVE_IsValid( drive )) return DRIVE_NO_ROOT_DIR;
    return DOSDrives[drive].type;
}


/***********************************************************************
 *           DRIVE_GetFlags
 */
UINT DRIVE_GetFlags( int drive )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES)) return 0;
    return DOSDrives[drive].flags;
}

/***********************************************************************
 *           DRIVE_Chdir
 */
int DRIVE_Chdir( int drive, LPCWSTR path )
{
    DOS_FULL_NAME full_name;
    WCHAR buffer[MAX_PATHNAME_LEN];
    LPSTR unix_cwd;
    BY_HANDLE_FILE_INFORMATION info;
    TDB *pTask = GlobalLock16(GetCurrentTask());

    buffer[0] = 'A' + drive;
    buffer[1] = ':';
    buffer[2] = 0;
    TRACE("(%s,%s)\n", debugstr_w(buffer), debugstr_w(path) );
    strncpyW( buffer + 2, path, MAX_PATHNAME_LEN - 2 );
    buffer[MAX_PATHNAME_LEN - 1] = 0; /* ensure 0 termination */

    if (!DOSFS_GetFullName( buffer, TRUE, &full_name )) return 0;
    if (!FILE_Stat( full_name.long_name, &info, NULL )) return 0;
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }
    unix_cwd = full_name.long_name + strlen( DOSDrives[drive].root );
    while (*unix_cwd == '/') unix_cwd++;

    TRACE("(%c:): unix_cwd=%s dos_cwd=%s\n",
            'A' + drive, unix_cwd, debugstr_w(full_name.short_name + 3) );

    HeapFree( GetProcessHeap(), 0, DOSDrives[drive].dos_cwd );
    HeapFree( GetProcessHeap(), 0, DOSDrives[drive].unix_cwd );
    DOSDrives[drive].dos_cwd  = HeapAlloc(GetProcessHeap(), 0, (strlenW(full_name.short_name) - 2) * sizeof(WCHAR));
    strcpyW(DOSDrives[drive].dos_cwd, full_name.short_name + 3);
    DOSDrives[drive].unix_cwd = heap_strdup( unix_cwd );

    if (drive == DRIVE_CurDrive)
    {
        UNICODE_STRING dirW;

        RtlInitUnicodeString( &dirW, full_name.short_name );
        RtlSetCurrentDirectory_U( &dirW );
    }

    if (pTask && (pTask->curdrive & 0x80) &&
        ((pTask->curdrive & ~0x80) == drive))
    {
        WideCharToMultiByte(CP_ACP, 0, full_name.short_name + 2, -1,
                            pTask->curdir, sizeof(pTask->curdir), NULL, NULL);
        DRIVE_LastTask = GetCurrentTask();
    }
    return 1;
}


/***********************************************************************
 *           DRIVE_GetFreeSpace
 */
static int DRIVE_GetFreeSpace( int drive, PULARGE_INTEGER size,
			       PULARGE_INTEGER available )
{
    struct statvfs info;

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return 0;
    }

    if (statvfs( DOSDrives[drive].root, &info ) < 0)
    {
        FILE_SetDosError();
        WARN("cannot do statvfs(%s)\n", DOSDrives[drive].root);
        return 0;
    }
    size->QuadPart = RtlEnlargedUnsignedMultiply( info.f_frsize, info.f_blocks );
    if (DOSDrives[drive].type == DRIVE_CDROM)
        available->QuadPart = 0; /* ALWAYS 0, even if no real CD-ROM mounted there !! */
    else
        available->QuadPart = RtlEnlargedUnsignedMultiply( info.f_frsize, info.f_bavail );

    return 1;
}

/***********************************************************************
 *       DRIVE_GetCurrentDirectory
 * Returns "X:\\path\\etc\\".
 *
 * Despite the API description, return required length including the
 * terminating null when buffer too small. This is the real behaviour.
*/
static UINT DRIVE_GetCurrentDirectory( UINT buflen, LPWSTR buf )
{
    UINT ret;
    LPCWSTR dos_cwd = DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() );
    static const WCHAR driveA_rootW[] = {'A',':','\\',0};

    ret = strlenW(dos_cwd) + 3; /* length of WHOLE current directory */
    if (ret >= buflen) return ret + 1;

    strcpyW( buf, driveA_rootW );
    buf[0] += DRIVE_GetCurrentDrive();
    strcatW( buf, dos_cwd );
    return ret;
}


/***********************************************************************
 *           DRIVE_BuildEnv
 *
 * Build the environment array containing the drives' current directories.
 * Resulting pointer must be freed with HeapFree.
 */
WCHAR *DRIVE_BuildEnv(void)
{
    int i, length = 0;
    LPCWSTR cwd[MAX_DOS_DRIVES];
    WCHAR *env, *p;

    for (i = 0; i < MAX_DOS_DRIVES; i++)
    {
        if ((cwd[i] = DRIVE_GetDosCwd(i)) && cwd[i][0])
            length += strlenW(cwd[i]) + 8;
    }
    if (!(env = HeapAlloc( GetProcessHeap(), 0, (length+1) * sizeof(WCHAR) ))) return NULL;
    for (i = 0, p = env; i < MAX_DOS_DRIVES; i++)
    {
        if (cwd[i] && cwd[i][0])
        {
            *p++ = '='; *p++ = 'A' + i; *p++ = ':';
            *p++ = '='; *p++ = 'A' + i; *p++ = ':'; *p++ = '\\';
            strcpyW( p, cwd[i] );
            p += strlenW(p) + 1;
        }
    }
    *p = 0;
    return env;
}


/***********************************************************************
 *           GetDiskFreeSpaceW   (KERNEL32.@)
 *
 * Fails if expression resulting from current drive's dir and "root"
 * is not a root dir of the target drive.
 *
 * UNDOC: setting some LPDWORDs to NULL is perfectly possible
 * if the corresponding info is unneeded.
 *
 * FIXME: needs to support UNC names from Win95 OSR2 on.
 *
 * Behaviour under Win95a:
 * CurrDir     root   result
 * "E:\\TEST"  "E:"   FALSE
 * "E:\\"      "E:"   TRUE
 * "E:\\"      "E"    FALSE
 * "E:\\"      "\\"   TRUE
 * "E:\\TEST"  "\\"   TRUE
 * "E:\\TEST"  ":\\"  FALSE
 * "E:\\TEST"  "E:\\" TRUE
 * "E:\\TEST"  ""     FALSE
 * "E:\\"      ""     FALSE (!)
 * "E:\\"      0x0    TRUE
 * "E:\\TEST"  0x0    TRUE  (!)
 * "E:\\TEST"  "C:"   TRUE  (when CurrDir of "C:" set to "\\")
 * "E:\\TEST"  "C:"   FALSE (when CurrDir of "C:" set to "\\TEST")
 */
BOOL WINAPI GetDiskFreeSpaceW( LPCWSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    int	drive, sec_size;
    ULARGE_INTEGER size,available;
    LPCWSTR path;
    DWORD cluster_sec;

    TRACE("%s,%p,%p,%p,%p\n", debugstr_w(root), cluster_sectors, sector_bytes,
          free_clusters, total_clusters);

    if (!root || root[0] == '\\' || root[0] == '/')
	drive = DRIVE_GetCurrentDrive();
    else
    if (root[0] && root[1] == ':') /* root contains drive tag */
    {
        drive = toupperW(root[0]) - 'A';
	path = &root[2];
	if (path[0] == '\0')
        {
	    path = DRIVE_GetDosCwd(drive);
            if (!path)
            {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
            }
        }
	else
	if (path[0] == '\\')
	    path++;

        if (path[0]) /* oops, we are in a subdir */
        {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        }
    }
    else
    {
        if (!root[0])
            SetLastError(ERROR_PATH_NOT_FOUND);
        else
            SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;

    /* Cap the size and available at 2GB as per specs.  */
    if ((size.u.HighPart) ||(size.u.LowPart > 0x7fffffff))
    {
	size.u.HighPart = 0;
	size.u.LowPart = 0x7fffffff;
    }
    if ((available.u.HighPart) ||(available.u.LowPart > 0x7fffffff))
    {
	available.u.HighPart =0;
	available.u.LowPart = 0x7fffffff;
    }
    sec_size = (DRIVE_GetType(drive)==DRIVE_CDROM) ? 2048 : 512;
    size.u.LowPart            /= sec_size;
    available.u.LowPart       /= sec_size;
    /* FIXME: probably have to adjust those variables too for CDFS */
    cluster_sec = 1;
    while (cluster_sec * 65536 < size.u.LowPart) cluster_sec *= 2;

    if (cluster_sectors)
	*cluster_sectors = cluster_sec;
    if (sector_bytes)
	*sector_bytes    = sec_size;
    if (free_clusters)
	*free_clusters   = available.u.LowPart / cluster_sec;
    if (total_clusters)
	*total_clusters  = size.u.LowPart / cluster_sec;
    return TRUE;
}


/***********************************************************************
 *           GetDiskFreeSpaceA   (KERNEL32.@)
 */
BOOL WINAPI GetDiskFreeSpaceA( LPCSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    UNICODE_STRING rootW;
    BOOL ret = FALSE;

    if (root)
    {
        if(!RtlCreateUnicodeStringFromAsciiz(&rootW, root))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    else
        rootW.Buffer = NULL;

    ret = GetDiskFreeSpaceW(rootW.Buffer, cluster_sectors, sector_bytes,
                            free_clusters, total_clusters );
    RtlFreeUnicodeString(&rootW);

    return ret;
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
BOOL WINAPI GetDiskFreeSpaceExW( LPCWSTR root,
				     PULARGE_INTEGER avail,
				     PULARGE_INTEGER total,
				     PULARGE_INTEGER totalfree)
{
    int	drive;
    ULARGE_INTEGER size,available;

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    { /* C: always works for GetDiskFreeSpaceEx */
        if ((root[1]) && ((root[1] != ':') || (root[2] && root[2] != '\\')))
        {
            FIXME("there are valid root names which are not supported yet\n");
	    /* ..like UNC names, for instance. */

            WARN("invalid root '%s'\n", debugstr_w(root));
            return FALSE;
        }
        drive = toupperW(root[0]) - 'A';
    }

    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;

    if (total)
    {
        total->u.HighPart = size.u.HighPart;
        total->u.LowPart = size.u.LowPart;
    }

    if (totalfree)
    {
        totalfree->u.HighPart = available.u.HighPart;
        totalfree->u.LowPart = available.u.LowPart;
    }

    if (avail)
    {
        if (FIXME_ON(dosfs))
	{
            /* On Windows2000, we need to check the disk quota
	       allocated for the user owning the calling process. We
	       don't want to be more obtrusive than necessary with the
	       FIXME messages, so don't print the FIXME unless Wine is
	       actually masquerading as Windows2000. */

            RTL_OSVERSIONINFOEXW ovi;
	    ovi.dwOSVersionInfoSize = sizeof(ovi);
	    if (RtlGetVersion(&ovi))
	    {
	      if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT && ovi.dwMajorVersion > 4)
                  FIXME("no per-user quota support yet\n");
	    }
	}

        /* Quick hack, should eventually be fixed to work 100% with
           Windows2000 (see comment above). */
        avail->u.HighPart = available.u.HighPart;
        avail->u.LowPart = available.u.LowPart;
    }

    return TRUE;
}

/***********************************************************************
 *           GetDiskFreeSpaceExA   (KERNEL32.@)
 */
BOOL WINAPI GetDiskFreeSpaceExA( LPCSTR root, PULARGE_INTEGER avail,
				     PULARGE_INTEGER total,
				     PULARGE_INTEGER  totalfree)
{
    UNICODE_STRING rootW;
    BOOL ret;

    if (root) RtlCreateUnicodeStringFromAsciiz(&rootW, root);
    else rootW.Buffer = NULL;

    ret = GetDiskFreeSpaceExW( rootW.Buffer, avail, total, totalfree);

    RtlFreeUnicodeString(&rootW);
    return ret;
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
 *   DRIVE_FIXED       the disk can not be removed from the machine
 *   DRIVE_REMOTE      network disk
 *   DRIVE_CDROM       CDROM drive
 *   DRIVE_RAMDISK     virtual disk in RAM
 */
UINT WINAPI GetDriveTypeW(LPCWSTR root) /* [in] String describing drive */
{
    int drive;
    TRACE("(%s)\n", debugstr_w(root));

    if (NULL == root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && (root[1] != ':'))
	{
	    WARN("invalid root %s\n", debugstr_w(root));
	    return DRIVE_NO_ROOT_DIR;
	}
	drive = toupperW(root[0]) - 'A';
    }
    return DRIVE_GetType(drive);
}


/***********************************************************************
 *           GetDriveTypeA   (KERNEL32.@)
 */
UINT WINAPI GetDriveTypeA( LPCSTR root )
{
    UNICODE_STRING rootW;
    UINT ret = 0;

    if (root)
    {
        if( !RtlCreateUnicodeStringFromAsciiz(&rootW, root))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }
    else
        rootW.Buffer = NULL;

    ret = GetDriveTypeW(rootW.Buffer);

    RtlFreeUnicodeString(&rootW);
    return ret;

}


/***********************************************************************
 *           GetCurrentDirectory   (KERNEL.411)
 */
UINT16 WINAPI GetCurrentDirectory16( UINT16 buflen, LPSTR buf )
{
    WCHAR cur_dirW[MAX_PATH];

    DRIVE_GetCurrentDirectory(MAX_PATH, cur_dirW);
    return (UINT16)WideCharToMultiByte(CP_ACP, 0, cur_dirW, -1, buf, buflen, NULL, NULL);
}


/***********************************************************************
 *           GetCurrentDirectoryW   (KERNEL32.@)
 */
UINT WINAPI GetCurrentDirectoryW( UINT buflen, LPWSTR buf )
{
    UINT ret;
    WCHAR longname[MAX_PATHNAME_LEN];
    WCHAR shortname[MAX_PATHNAME_LEN];

    ret = DRIVE_GetCurrentDirectory(MAX_PATHNAME_LEN, shortname);
    if ( ret > MAX_PATHNAME_LEN ) {
      ERR_(file)("pathnamelength (%d) > MAX_PATHNAME_LEN!\n", ret );
      return ret;
    }
    GetLongPathNameW(shortname, longname, MAX_PATHNAME_LEN);
    ret = strlenW( longname ) + 1;
    if (ret > buflen) return ret;
    strcpyW(buf, longname);
    return ret - 1;
}

/***********************************************************************
 *           GetCurrentDirectoryA   (KERNEL32.@)
 */
UINT WINAPI GetCurrentDirectoryA( UINT buflen, LPSTR buf )
{
    WCHAR bufferW[MAX_PATH];
    DWORD ret, retW;

    retW = GetCurrentDirectoryW(MAX_PATH, bufferW);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        if (buflen >= ret)
        {
            WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buf, buflen, NULL, NULL);
            ret--; /* length without 0 */
        }
    }
    return ret;
}


/***********************************************************************
 *           SetCurrentDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI SetCurrentDirectoryW( LPCWSTR dir )
{
    int drive, olddrive = DRIVE_GetCurrentDrive();

    if (!dir)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if (dir[0] && (dir[1]==':'))
    {
        drive = toupperW( *dir ) - 'A';
        dir += 2;
    }
    else
	drive = olddrive;

    /* WARNING: we need to set the drive before the dir, as DRIVE_Chdir
       sets pTask->curdir only if pTask->curdrive is drive */
    if (!(DRIVE_SetCurrentDrive( drive )))
	return FALSE;

    /* FIXME: what about empty strings? Add a \\ ? */
    if (!DRIVE_Chdir( drive, dir )) {
	DRIVE_SetCurrentDrive(olddrive);
	return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           SetCurrentDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI SetCurrentDirectoryA( LPCSTR dir )
{
    UNICODE_STRING dirW;
    BOOL ret = FALSE;

    if (!dir)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&dirW, dir))
    {
        ret = SetCurrentDirectoryW(dirW.Buffer);
        RtlFreeUnicodeString(&dirW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
}


/***********************************************************************
 *           GetLogicalDriveStringsA   (KERNEL32.@)
 */
UINT WINAPI GetLogicalDriveStringsA( UINT len, LPSTR buffer )
{
    int drive, count;

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) count++;
    if ((count * 4) + 1 <= len)
    {
        LPSTR p = buffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (DRIVE_IsValid(drive))
            {
                *p++ = 'a' + drive;
                *p++ = ':';
                *p++ = '\\';
                *p++ = '\0';
            }
        *p = '\0';
        return count * 4;
    }
    else
        return (count * 4) + 1; /* account for terminating null */
    /* The API tells about these different return values */
}


/***********************************************************************
 *           GetLogicalDriveStringsW   (KERNEL32.@)
 */
UINT WINAPI GetLogicalDriveStringsW( UINT len, LPWSTR buffer )
{
    int drive, count;

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) count++;
    if (count * 4 * sizeof(WCHAR) <= len)
    {
        LPWSTR p = buffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (DRIVE_IsValid(drive))
            {
                *p++ = (WCHAR)('a' + drive);
                *p++ = (WCHAR)':';
                *p++ = (WCHAR)'\\';
                *p++ = (WCHAR)'\0';
            }
        *p = (WCHAR)'\0';
    }
    return count * 4 * sizeof(WCHAR);
}


/***********************************************************************
 *           GetLogicalDrives   (KERNEL32.@)
 */
DWORD WINAPI GetLogicalDrives(void)
{
    DWORD ret = 0;
    int drive;

    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if ( (DRIVE_IsValid(drive)) ||
            (DOSDrives[drive].type == DRIVE_CDROM)) /* audio CD is also valid */
            ret |= (1 << drive);
    }
    return ret;
}
