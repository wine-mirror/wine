/*
 * NTDLL directory functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 2003 Eric Pouech
 * Copyright 1996, 2004 Alexandre Julliard
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

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_IOCTL_H
#include <linux/ioctl.h>
#endif
#ifdef HAVE_LINUX_MAJOR_H
# include <linux/major.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "thread.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/* just in case... */
#undef VFAT_IOCTL_READDIR_BOTH
#undef USE_GETDENTS

#ifdef linux

/* We want the real kernel dirent structure, not the libc one */
typedef struct
{
    long d_ino;
    long d_off;
    unsigned short d_reclen;
    char d_name[256];
} KERNEL_DIRENT;

/* Define the VFAT ioctl to get both short and long file names */
#define VFAT_IOCTL_READDIR_BOTH  _IOR('r', 1, KERNEL_DIRENT [2] )

#ifndef O_DIRECTORY
# define O_DIRECTORY 0200000 /* must be directory */
#endif

#ifdef __i386__

typedef struct
{
    ULONG64        d_ino;
    LONG64         d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[256];
} KERNEL_DIRENT64;

static inline int getdents64( int fd, char *de, unsigned int size )
{
    int ret;
    __asm__( "pushl %%ebx; movl %2,%%ebx; int $0x80; popl %%ebx"
             : "=a" (ret)
             : "0" (220 /*NR_getdents64*/), "r" (fd), "c" (de), "d" (size)
             : "memory" );
    if (ret < 0)
    {
        errno = -ret;
        ret = -1;
    }
    return ret;
}
#define USE_GETDENTS

#endif  /* i386 */

#endif  /* linux */

#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_SEPARATOR(ch)   ((ch) == '\\' || (ch) == '/')

#define INVALID_NT_CHARS   '*','?','<','>','|','"'
#define INVALID_DOS_CHARS  INVALID_NT_CHARS,'+','=',',',';','[',']',' ','\345'

#define MAX_DIR_ENTRY_LEN 255  /* max length of a directory entry in chars */

static const unsigned int max_dir_info_size = FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileName[MAX_DIR_ENTRY_LEN] );

static int show_dot_files = -1;

/* at some point we may want to allow Winelib apps to set this */
static const int is_case_sensitive = FALSE;

static RTL_CRITICAL_SECTION dir_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &dir_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dir_section") }
};
static RTL_CRITICAL_SECTION dir_section = { &critsect_debug, -1, 0, 0, 0, 0 };


/* check if a given Unicode char is OK in a DOS short name */
static inline BOOL is_invalid_dos_char( WCHAR ch )
{
    static const WCHAR invalid_chars[] = { INVALID_DOS_CHARS,'~','.',0 };
    if (ch > 0x7f) return TRUE;
    return strchrW( invalid_chars, ch ) != NULL;
}

/* check if the device can be a mounted volume */
static inline int is_valid_mounted_device( struct stat *st )
{
#if defined(linux) || defined(__sun__)
    return S_ISBLK( st->st_mode );
#else
    /* disks are char devices on *BSD */
    return S_ISCHR( st->st_mode );
#endif
}

/***********************************************************************
 *           get_default_com_device
 *
 * Return the default device to use for serial ports.
 */
static char *get_default_com_device( int num )
{
    char *ret = NULL;

    if (!num || num > 9) return ret;
#ifdef linux
    ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof("/dev/ttyS0") );
    if (ret)
    {
        strcpy( ret, "/dev/ttyS0" );
        ret[strlen(ret) - 1] = '0' + num - 1;
    }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof("/dev/cuad0") );
    if (ret)
    {
        strcpy( ret, "/dev/cuad0" );
        ret[strlen(ret) - 1] = '0' + num - 1;
    }
#else
    FIXME( "no known default for device com%d\n", num );
#endif
    return ret;
}


/***********************************************************************
 *           get_default_lpt_device
 *
 * Return the default device to use for parallel ports.
 */
static char *get_default_lpt_device( int num )
{
    char *ret = NULL;

    if (!num || num > 9) return ret;
#ifdef linux
    ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof("/dev/lp0") );
    if (ret)
    {
        strcpy( ret, "/dev/lp0" );
        ret[strlen(ret) - 1] = '0' + num - 1;
    }
#else
    FIXME( "no known default for device lpt%d\n", num );
#endif
    return ret;
}


/***********************************************************************
 *           parse_mount_entries
 *
 * Parse mount entries looking for a given device. Helper for get_default_drive_device.
 */

#ifdef sun
#include <sys/vfstab.h>
static char *parse_vfstab_entries( FILE *f, dev_t dev, ino_t ino)
{
    struct vfstab entry;
    struct stat st;
    char *device;

    while (! getvfsent( f, &entry ))
    {
        /* don't even bother stat'ing network mounts, there's no meaningful device anyway */
        if (!strcmp( entry.vfs_fstype, "nfs" ) ||
            !strcmp( entry.vfs_fstype, "smbfs" ) ||
            !strcmp( entry.vfs_fstype, "ncpfs" )) continue;

        if (stat( entry.vfs_mountp, &st ) == -1) continue;
        if (st.st_dev != dev || st.st_ino != ino) continue;
        if (!strcmp( entry.vfs_fstype, "fd" ))
        {
            if ((device = strstr( entry.vfs_mntopts, "dev=" )))
            {
                char *p = strchr( device + 4, ',' );
                if (p) *p = 0;
                return device + 4;
            }
        }
        else
            return entry.vfs_special;
    }
    return NULL;
}
#endif

#ifdef linux
static char *parse_mount_entries( FILE *f, dev_t dev, ino_t ino )
{
    struct mntent *entry;
    struct stat st;
    char *device;

    while ((entry = getmntent( f )))
    {
        /* don't even bother stat'ing network mounts, there's no meaningful device anyway */
        if (!strcmp( entry->mnt_type, "nfs" ) ||
            !strcmp( entry->mnt_type, "smbfs" ) ||
            !strcmp( entry->mnt_type, "ncpfs" )) continue;

        if (stat( entry->mnt_dir, &st ) == -1) continue;
        if (st.st_dev != dev || st.st_ino != ino) continue;
        if (!strcmp( entry->mnt_type, "supermount" ))
        {
            if ((device = strstr( entry->mnt_opts, "dev=" )))
            {
                char *p = strchr( device + 4, ',' );
                if (p) *p = 0;
                return device + 4;
            }
        }
        else if (!stat( entry->mnt_fsname, &st ) && S_ISREG(st.st_mode))
        {
            /* if device is a regular file check for a loop mount */
            if ((device = strstr( entry->mnt_opts, "loop=" )))
            {
                char *p = strchr( device + 5, ',' );
                if (p) *p = 0;
                return device + 5;
            }
        }
        else
            return entry->mnt_fsname;
    }
    return NULL;
}
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <fstab.h>
static char *parse_mount_entries( FILE *f, dev_t dev, ino_t ino )
{
    struct fstab *entry;
    struct stat st;

    while ((entry = getfsent()))
    {
        /* don't even bother stat'ing network mounts, there's no meaningful device anyway */
        if (!strcmp( entry->fs_vfstype, "nfs" ) ||
            !strcmp( entry->fs_vfstype, "smbfs" ) ||
            !strcmp( entry->fs_vfstype, "ncpfs" )) continue;

        if (stat( entry->fs_file, &st ) == -1) continue;
        if (st.st_dev != dev || st.st_ino != ino) continue;
        return entry->fs_spec;
    }
    return NULL;
}
#endif

#ifdef sun
#include <sys/mnttab.h>
static char *parse_mount_entries( FILE *f, dev_t dev, ino_t ino )
{
    struct mnttab entry;
    struct stat st;
    char *device;


    while (( ! getmntent( f, &entry) ))
    {
        /* don't even bother stat'ing network mounts, there's no meaningful device anyway */
        if (!strcmp( entry.mnt_fstype, "nfs" ) ||
            !strcmp( entry.mnt_fstype, "smbfs" ) ||
            !strcmp( entry.mnt_fstype, "ncpfs" )) continue;

        if (stat( entry.mnt_mountp, &st ) == -1) continue;
        if (st.st_dev != dev || st.st_ino != ino) continue;
        if (!strcmp( entry.mnt_fstype, "fd" ))
        {
            if ((device = strstr( entry.mnt_mntopts, "dev=" )))
            {
                char *p = strchr( device + 4, ',' );
                if (p) *p = 0;
                return device + 4;
            }
        }
        else
            return entry.mnt_special;
    }
    return NULL;
}
#endif

/***********************************************************************
 *           get_default_drive_device
 *
 * Return the default device to use for a given drive mount point.
 */
static char *get_default_drive_device( const char *root )
{
    char *ret = NULL;

#ifdef linux
    FILE *f;
    char *device = NULL;
    int fd, res = -1;
    struct stat st;

    /* try to open it first to force it to get mounted */
    if ((fd = open( root, O_RDONLY | O_DIRECTORY )) != -1)
    {
        res = fstat( fd, &st );
        close( fd );
    }
    /* now try normal stat just in case */
    if (res == -1) res = stat( root, &st );
    if (res == -1) return NULL;

    RtlEnterCriticalSection( &dir_section );

    if ((f = fopen( "/etc/mtab", "r" )))
    {
        device = parse_mount_entries( f, st.st_dev, st.st_ino );
        endmntent( f );
    }
    /* look through fstab too in case it's not mounted (for instance if it's an audio CD) */
    if (!device && (f = fopen( "/etc/fstab", "r" )))
    {
        device = parse_mount_entries( f, st.st_dev, st.st_ino );
        endmntent( f );
    }
    if (device)
    {
        ret = RtlAllocateHeap( GetProcessHeap(), 0, strlen(device) + 1 );
        if (ret) strcpy( ret, device );
    }
    RtlLeaveCriticalSection( &dir_section );

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__ )
    char *device = NULL;
    int fd, res = -1;
    struct stat st;

    /* try to open it first to force it to get mounted */
    if ((fd = open( root, O_RDONLY )) != -1)
    {
        res = fstat( fd, &st );
        close( fd );
    }
    /* now try normal stat just in case */
    if (res == -1) res = stat( root, &st );
    if (res == -1) return NULL;

    RtlEnterCriticalSection( &dir_section );

    /* The FreeBSD parse_mount_entries doesn't require a file argument, so just
     * pass NULL.  Leave the argument in for symmetry.
     */
    device = parse_mount_entries( NULL, st.st_dev, st.st_ino );
    if (device)
    {
        ret = RtlAllocateHeap( GetProcessHeap(), 0, strlen(device) + 1 );
        if (ret) strcpy( ret, device );
    }
    RtlLeaveCriticalSection( &dir_section );

#elif defined( sun )
    FILE *f;
    char *device = NULL;
    int fd, res = -1;
    struct stat st;

    /* try to open it first to force it to get mounted */
    if ((fd = open( root, O_RDONLY )) != -1)
    {
        res = fstat( fd, &st );
        close( fd );
    }
    /* now try normal stat just in case */
    if (res == -1) res = stat( root, &st );
    if (res == -1) return NULL;

    RtlEnterCriticalSection( &dir_section );

    if ((f = fopen( "/etc/mnttab", "r" )))
    {
        device = parse_mount_entries( f, st.st_dev, st.st_ino);
        fclose( f );
    }
    /* look through fstab too in case it's not mounted (for instance if it's an audio CD) */
    if (!device && (f = fopen( "/etc/vfstab", "r" )))
    {
        device = parse_vfstab_entries( f, st.st_dev, st.st_ino );
        fclose( f );
    }
    if (device)
    {
        ret = RtlAllocateHeap( GetProcessHeap(), 0, strlen(device) + 1 );
        if (ret) strcpy( ret, device );
    }
    RtlLeaveCriticalSection( &dir_section );

#elif defined(__APPLE__)
    struct statfs *mntStat;
    struct stat st;
    int i;
    int mntSize;
    dev_t dev;
    ino_t ino;
    static const char path_bsd_device[] = "/dev/disk";
    int res;

    res = stat( root, &st );
    if (res == -1) return NULL;

    dev = st.st_dev;
    ino = st.st_ino;

    RtlEnterCriticalSection( &dir_section );

    mntSize = getmntinfo(&mntStat, MNT_NOWAIT);

    for (i = 0; i < mntSize && !ret; i++)
    {
        if (stat(mntStat[i].f_mntonname, &st ) == -1) continue;
        if (st.st_dev != dev || st.st_ino != ino) continue;

        /* FIXME add support for mounted network drive */
        if ( strncmp(mntStat[i].f_mntfromname, path_bsd_device, strlen(path_bsd_device)) == 0)
        {
            /* set return value to the corresponding raw BSD node */
            ret = RtlAllocateHeap( GetProcessHeap(), 0, strlen(mntStat[i].f_mntfromname) + 2 /* 2 : r and \0 */ );
            if (ret)
            {
                strcpy(ret, "/dev/r");
                strcat(ret, mntStat[i].f_mntfromname+sizeof("/dev/")-1);
            }
        }
    }
    RtlLeaveCriticalSection( &dir_section );
#else
    static int warned;
    if (!warned++) FIXME( "auto detection of DOS devices not supported on this platform\n" );
#endif
    return ret;
}


/***********************************************************************
 *           get_device_mount_point
 *
 * Return the current mount point for a device.
 */
static char *get_device_mount_point( dev_t dev )
{
    char *ret = NULL;

#ifdef linux
    FILE *f;

    RtlEnterCriticalSection( &dir_section );

    if ((f = fopen( "/etc/mtab", "r" )))
    {
        struct mntent *entry;
        struct stat st;
        char *p, *device;

        while ((entry = getmntent( f )))
        {
            /* don't even bother stat'ing network mounts, there's no meaningful device anyway */
            if (!strcmp( entry->mnt_type, "nfs" ) ||
                !strcmp( entry->mnt_type, "smbfs" ) ||
                !strcmp( entry->mnt_type, "ncpfs" )) continue;

            if (!strcmp( entry->mnt_type, "supermount" ))
            {
                if ((device = strstr( entry->mnt_opts, "dev=" )))
                {
                    device += 4;
                    if ((p = strchr( device, ',' ))) *p = 0;
                }
            }
            else if (!stat( entry->mnt_fsname, &st ) && S_ISREG(st.st_mode))
            {
                /* if device is a regular file check for a loop mount */
                if ((device = strstr( entry->mnt_opts, "loop=" )))
                {
                    device += 5;
                    if ((p = strchr( device, ',' ))) *p = 0;
                }
            }
            else device = entry->mnt_fsname;

            if (device && !stat( device, &st ) && S_ISBLK(st.st_mode) && st.st_rdev == dev)
            {
                ret = RtlAllocateHeap( GetProcessHeap(), 0, strlen(entry->mnt_dir) + 1 );
                if (ret) strcpy( ret, entry->mnt_dir );
                break;
            }
        }
        endmntent( f );
    }
    RtlLeaveCriticalSection( &dir_section );
#elif defined(__APPLE__)
    struct statfs *entry;
    struct stat st;
    int i, size;

    RtlEnterCriticalSection( &dir_section );

    size = getmntinfo( &entry, MNT_NOWAIT );
    for (i = 0; i < size; i++)
    {
        if (stat( entry[i].f_mntfromname, &st ) == -1) continue;
        if (S_ISBLK(st.st_mode) && st.st_rdev == dev)
        {
            ret = RtlAllocateHeap( GetProcessHeap(), 0, strlen(entry[i].f_mntfromname) + 1 );
            if (ret) strcpy( ret, entry[i].f_mntfromname );
            break;
        }
    }
    RtlLeaveCriticalSection( &dir_section );
#else
    static int warned;
    if (!warned++) FIXME( "unmounting devices not supported on this platform\n" );
#endif
    return ret;
}


/***********************************************************************
 *           init_options
 *
 * Initialize the show_dot_files options.
 */
static void init_options(void)
{
    static const WCHAR WineW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e',0};
    static const WCHAR ShowDotFilesW[] = {'S','h','o','w','D','o','t','F','i','l','e','s',0};
    char tmp[80];
    HANDLE root, hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    show_dot_files = 0;

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, WineW );

    /* @@ Wine registry key: HKCU\Software\Wine */
    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        RtlInitUnicodeString( &nameW, ShowDotFilesW );
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            show_dot_files = IS_OPTION_TRUE( str[0] );
        }
        NtClose( hkey );
    }
    NtClose( root );
}


/***********************************************************************
 *           DIR_is_hidden_file
 *
 * Check if the specified file should be hidden based on its name and the show dot files option.
 */
BOOL DIR_is_hidden_file( const UNICODE_STRING *name )
{
    WCHAR *p, *end;

    if (show_dot_files == -1) init_options();
    if (show_dot_files) return FALSE;

    end = p = name->Buffer + name->Length/sizeof(WCHAR);
    while (p > name->Buffer && IS_SEPARATOR(p[-1])) p--;
    while (p > name->Buffer && !IS_SEPARATOR(p[-1])) p--;
    if (p == end || *p != '.') return FALSE;
    /* make sure it isn't '.' or '..' */
    if (p + 1 == end) return FALSE;
    if (p[1] == '.' && p + 2 == end) return FALSE;
    return TRUE;
}


/***********************************************************************
 *           hash_short_file_name
 *
 * Transform a Unix file name into a hashed DOS name. If the name is a valid
 * DOS name, it is converted to upper-case; otherwise it is replaced by a
 * hashed version that fits in 8.3 format.
 * 'buffer' must be at least 12 characters long.
 * Returns length of short name in bytes; short name is NOT null-terminated.
 */
static ULONG hash_short_file_name( const UNICODE_STRING *name, LPWSTR buffer )
{
    static const char hash_chars[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    LPCWSTR p, ext, end = name->Buffer + name->Length / sizeof(WCHAR);
    LPWSTR dst;
    unsigned short hash;
    int i;

    /* Compute the hash code of the file name */
    /* If you know something about hash functions, feel free to */
    /* insert a better algorithm here... */
    if (!is_case_sensitive)
    {
        for (p = name->Buffer, hash = 0xbeef; p < end - 1; p++)
            hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p) ^ (tolowerW(p[1]) << 8);
        hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p); /* Last character */
    }
    else
    {
        for (p = name->Buffer, hash = 0xbeef; p < end - 1; p++)
            hash = (hash << 3) ^ (hash >> 5) ^ *p ^ (p[1] << 8);
        hash = (hash << 3) ^ (hash >> 5) ^ *p;  /* Last character */
    }

    /* Find last dot for start of the extension */
    for (p = name->Buffer + 1, ext = NULL; p < end - 1; p++) if (*p == '.') ext = p;

    /* Copy first 4 chars, replacing invalid chars with '_' */
    for (i = 4, p = name->Buffer, dst = buffer; i > 0; i--, p++)
    {
        if (p == end || p == ext) break;
        *dst++ = is_invalid_dos_char(*p) ? '_' : toupperW(*p);
    }
    /* Pad to 5 chars with '~' */
    while (i-- >= 0) *dst++ = '~';

    /* Insert hash code converted to 3 ASCII chars */
    *dst++ = hash_chars[(hash >> 10) & 0x1f];
    *dst++ = hash_chars[(hash >> 5) & 0x1f];
    *dst++ = hash_chars[hash & 0x1f];

    /* Copy the first 3 chars of the extension (if any) */
    if (ext)
    {
        *dst++ = '.';
        for (i = 3, ext++; (i > 0) && ext < end; i--, ext++)
            *dst++ = is_invalid_dos_char(*ext) ? '_' : toupperW(*ext);
    }
    return dst - buffer;
}


/***********************************************************************
 *           match_filename
 *
 * Check a long file name against a mask.
 *
 * Tests (done in W95 DOS shell - case insensitive):
 * *.txt			test1.test.txt				*
 * *st1*			test1.txt				*
 * *.t??????.t*			test1.ta.tornado.txt			*
 * *tornado*			test1.ta.tornado.txt			*
 * t*t				test1.ta.tornado.txt			*
 * ?est*			test1.txt				*
 * ?est???			test1.txt				-
 * *test1.txt*			test1.txt				*
 * h?l?o*t.dat			hellothisisatest.dat			*
 */
static BOOLEAN match_filename( const UNICODE_STRING *name_str, const UNICODE_STRING *mask_str )
{
    int mismatch;
    const WCHAR *name = name_str->Buffer;
    const WCHAR *mask = mask_str->Buffer;
    const WCHAR *name_end = name + name_str->Length / sizeof(WCHAR);
    const WCHAR *mask_end = mask + mask_str->Length / sizeof(WCHAR);
    const WCHAR *lastjoker = NULL;
    const WCHAR *next_to_retry = NULL;

    TRACE("(%s, %s)\n", debugstr_us(name_str), debugstr_us(mask_str));

    while (name < name_end && mask < mask_end)
    {
        switch(*mask)
        {
        case '*':
            mask++;
            while (mask < mask_end && *mask == '*') mask++;  /* Skip consecutive '*' */
            if (mask == mask_end) return TRUE; /* end of mask is all '*', so match */
            lastjoker = mask;

            /* skip to the next match after the joker(s) */
            if (is_case_sensitive)
                while (name < name_end && (*name != *mask)) name++;
            else
                while (name < name_end && (toupperW(*name) != toupperW(*mask))) name++;
            next_to_retry = name;
            break;
        case '?':
            mask++;
            name++;
            break;
        default:
            if (is_case_sensitive) mismatch = (*mask != *name);
            else mismatch = (toupperW(*mask) != toupperW(*name));

            if (!mismatch)
            {
                mask++;
                name++;
                if (mask == mask_end)
                {
                    if (name == name_end) return TRUE;
                    if (lastjoker) mask = lastjoker;
                }
            }
            else /* mismatch ! */
            {
                if (lastjoker) /* we had an '*', so we can try unlimitedly */
                {
                    mask = lastjoker;

                    /* this scan sequence was a mismatch, so restart
                     * 1 char after the first char we checked last time */
                    next_to_retry++;
                    name = next_to_retry;
                }
                else return FALSE; /* bad luck */
            }
            break;
        }
    }
    while (mask < mask_end && ((*mask == '.') || (*mask == '*')))
        mask++;  /* Ignore trailing '.' or '*' in mask */
    return (name == name_end && mask == mask_end);
}


/***********************************************************************
 *           append_entry
 *
 * helper for NtQueryDirectoryFile
 */
static FILE_BOTH_DIR_INFORMATION *append_entry( void *info_ptr, ULONG_PTR *pos, ULONG max_length,
                                                const char *long_name, const char *short_name,
                                                const UNICODE_STRING *mask )
{
    FILE_BOTH_DIR_INFORMATION *info;
    int i, long_len, short_len, total_len;
    struct stat st;
    WCHAR long_nameW[MAX_DIR_ENTRY_LEN];
    WCHAR short_nameW[12];
    UNICODE_STRING str;

    long_len = ntdll_umbstowcs( 0, long_name, strlen(long_name), long_nameW, MAX_DIR_ENTRY_LEN );
    if (long_len == -1) return NULL;

    str.Buffer = long_nameW;
    str.Length = long_len * sizeof(WCHAR);
    str.MaximumLength = sizeof(long_nameW);

    if (short_name)
    {
        short_len = ntdll_umbstowcs( 0, short_name, strlen(short_name),
                                     short_nameW, sizeof(short_nameW) / sizeof(WCHAR) );
        if (short_len == -1) short_len = sizeof(short_nameW) / sizeof(WCHAR);
    }
    else  /* generate a short name if necessary */
    {
        BOOLEAN spaces;

        short_len = 0;
        if (!RtlIsNameLegalDOS8Dot3( &str, NULL, &spaces ) || spaces)
            short_len = hash_short_file_name( &str, short_nameW );
    }

    TRACE( "long %s short %s mask %s\n",
           debugstr_us(&str), debugstr_wn(short_nameW, short_len), debugstr_us(mask) );

    if (mask && !match_filename( &str, mask ))
    {
        if (!short_len) return NULL;  /* no short name to match */
        str.Buffer = short_nameW;
        str.Length = short_len * sizeof(WCHAR);
        str.MaximumLength = sizeof(short_nameW);
        if (!match_filename( &str, mask )) return NULL;
    }

    total_len = (sizeof(*info) - sizeof(info->FileName) + long_len*sizeof(WCHAR) + 3) & ~3;
    info = (FILE_BOTH_DIR_INFORMATION *)((char *)info_ptr + *pos);

    if (*pos + total_len > max_length) total_len = max_length - *pos;

    info->FileAttributes = 0;
    if (lstat( long_name, &st ) == -1) return NULL;
    if (S_ISLNK( st.st_mode ))
    {
        if (stat( long_name, &st ) == -1) return NULL;
        if (S_ISDIR( st.st_mode )) info->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
    }

    info->NextEntryOffset = total_len;
    info->FileIndex = 0;  /* NTFS always has 0 here, so let's not bother with it */

    RtlSecondsSince1970ToTime( st.st_mtime, &info->CreationTime );
    RtlSecondsSince1970ToTime( st.st_mtime, &info->LastWriteTime );
    RtlSecondsSince1970ToTime( st.st_atime, &info->LastAccessTime );
    RtlSecondsSince1970ToTime( st.st_ctime, &info->ChangeTime );

    if (S_ISDIR(st.st_mode))
    {
        info->EndOfFile.QuadPart = info->AllocationSize.QuadPart = 0;
        info->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
        info->EndOfFile.QuadPart = st.st_size;
        info->AllocationSize.QuadPart = (ULONGLONG)st.st_blocks * 512;
        info->FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
    }

    if (!(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
        info->FileAttributes |= FILE_ATTRIBUTE_READONLY;

    if (!show_dot_files && long_name[0] == '.' && long_name[1] && (long_name[1] != '.' || long_name[2]))
        info->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;

    info->EaSize = 0; /* FIXME */
    info->ShortNameLength = short_len * sizeof(WCHAR);
    for (i = 0; i < short_len; i++) info->ShortName[i] = toupperW(short_nameW[i]);
    info->FileNameLength = long_len * sizeof(WCHAR);
    memcpy( info->FileName, long_nameW,
            min( info->FileNameLength, total_len-sizeof(*info)+sizeof(info->FileName) ));

    *pos += total_len;
    return info;
}


#ifdef VFAT_IOCTL_READDIR_BOTH

/***********************************************************************
 *           start_vfat_ioctl
 *
 * Wrapper for the VFAT ioctl to work around various kernel bugs.
 * dir_section must be held by caller.
 */
static KERNEL_DIRENT *start_vfat_ioctl( int fd )
{
    static KERNEL_DIRENT *de;
    int res;

    if (!de)
    {
        const size_t page_size = getpagesize();
        SIZE_T size = 2 * sizeof(*de) + page_size;
        void *addr = NULL;

        if (NtAllocateVirtualMemory( GetCurrentProcess(), &addr, 1, &size, MEM_RESERVE, PAGE_READWRITE ))
            return NULL;
        /* commit only the size needed for the dir entries */
        /* this leaves an extra unaccessible page, which should make the kernel */
        /* fail with -EFAULT before it stomps all over our memory */
        de = addr;
        size = 2 * sizeof(*de);
        NtAllocateVirtualMemory( GetCurrentProcess(), &addr, 1, &size, MEM_COMMIT, PAGE_READWRITE );
    }

    /* set d_reclen to 65535 to work around an AFS kernel bug */
    de[0].d_reclen = 65535;
    res = ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de );
    if (res == -1)
    {
        if (errno != ENOENT) return NULL;  /* VFAT ioctl probably not supported */
        de[0].d_reclen = 0;  /* eof */
    }
    else if (!res && de[0].d_reclen == 65535) return NULL;  /* AFS bug */

    return de;
}


/***********************************************************************
 *           read_directory_vfat
 *
 * Read a directory using the VFAT ioctl; helper for NtQueryDirectoryFile.
 */
static int read_directory_vfat( int fd, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                BOOLEAN single_entry, const UNICODE_STRING *mask,
                                BOOLEAN restart_scan )

{
    size_t len;
    KERNEL_DIRENT *de;
    FILE_BOTH_DIR_INFORMATION *info, *last_info = NULL;

    io->u.Status = STATUS_SUCCESS;

    if (restart_scan) lseek( fd, 0, SEEK_SET );

    if (length < max_dir_info_size)  /* we may have to return a partial entry here */
    {
        off_t old_pos = lseek( fd, 0, SEEK_CUR );

        if (!(de = start_vfat_ioctl( fd ))) return -1;  /* not supported */

        while (de[0].d_reclen)
        {
            /* make sure names are null-terminated to work around an x86-64 kernel bug */
            len = min(de[0].d_reclen, sizeof(de[0].d_name) - 1 );
            de[0].d_name[len] = 0;
            len = min(de[1].d_reclen, sizeof(de[1].d_name) - 1 );
            de[1].d_name[len] = 0;

            if (de[1].d_name[0])
                info = append_entry( buffer, &io->Information, length,
                                     de[1].d_name, de[0].d_name, mask );
            else
                info = append_entry( buffer, &io->Information, length,
                                     de[0].d_name, NULL, mask );
            if (info)
            {
                last_info = info;
                if ((char *)info->FileName + info->FileNameLength > (char *)buffer + length)
                {
                    io->u.Status = STATUS_BUFFER_OVERFLOW;
                    lseek( fd, old_pos, SEEK_SET );  /* restore pos to previous entry */
                }
                break;
            }
            old_pos = lseek( fd, 0, SEEK_CUR );
            if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) == -1) break;
        }
    }
    else  /* we'll only return full entries, no need to worry about overflow */
    {
        if (!(de = start_vfat_ioctl( fd ))) return -1;  /* not supported */

        while (de[0].d_reclen)
        {
            /* make sure names are null-terminated to work around an x86-64 kernel bug */
            len = min(de[0].d_reclen, sizeof(de[0].d_name) - 1 );
            de[0].d_name[len] = 0;
            len = min(de[1].d_reclen, sizeof(de[1].d_name) - 1 );
            de[1].d_name[len] = 0;

            if (de[1].d_name[0])
                info = append_entry( buffer, &io->Information, length,
                                     de[1].d_name, de[0].d_name, mask );
            else
                info = append_entry( buffer, &io->Information, length,
                                     de[0].d_name, NULL, mask );
            if (info)
            {
                last_info = info;
                if (single_entry) break;
                /* check if we still have enough space for the largest possible entry */
                if (io->Information + max_dir_info_size > length) break;
            }
            if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) == -1) break;
        }
    }

    if (last_info) last_info->NextEntryOffset = 0;
    else io->u.Status = restart_scan ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES;
    return 0;
}
#endif /* VFAT_IOCTL_READDIR_BOTH */


/***********************************************************************
 *           read_directory_getdents
 *
 * Read a directory using the Linux getdents64 system call; helper for NtQueryDirectoryFile.
 */
#ifdef USE_GETDENTS
static int read_directory_getdents( int fd, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                    BOOLEAN single_entry, const UNICODE_STRING *mask,
                                    BOOLEAN restart_scan )
{
    off_t old_pos = 0;
    size_t size = length;
    int res, fake_dot_dot = 1;
    char *data, local_buffer[8192];
    KERNEL_DIRENT64 *de;
    FILE_BOTH_DIR_INFORMATION *info, *last_info = NULL;

    if (size <= sizeof(local_buffer) || !(data = RtlAllocateHeap( GetProcessHeap(), 0, size )))
    {
        size = sizeof(local_buffer);
        data = local_buffer;
    }

    if (restart_scan) lseek( fd, 0, SEEK_SET );
    else if (length < max_dir_info_size)  /* we may have to return a partial entry here */
    {
        old_pos = lseek( fd, 0, SEEK_CUR );
        if (old_pos == -1 && errno == ENOENT)
        {
            io->u.Status = STATUS_NO_MORE_FILES;
            res = 0;
            goto done;
        }
    }

    io->u.Status = STATUS_SUCCESS;

    res = getdents64( fd, data, size );
    if (res == -1)
    {
        if (errno != ENOSYS)
        {
            io->u.Status = FILE_GetNtStatus();
            res = 0;
        }
        goto done;
    }

    de = (KERNEL_DIRENT64 *)data;

    if (restart_scan)
    {
        /* check if we got . and .. from getdents */
        if (res > 0)
        {
            if (!strcmp( de->d_name, "." ) && res > de->d_reclen)
            {
                KERNEL_DIRENT64 *next_de = (KERNEL_DIRENT64 *)(data + de->d_reclen);
                if (!strcmp( next_de->d_name, ".." )) fake_dot_dot = 0;
            }
        }
        /* make sure we have enough room for both entries */
        if (fake_dot_dot)
        {
            static const ULONG min_info_size = (FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName[1]) +
                                                FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName[2]) + 3) & ~3;
            if (length < min_info_size || single_entry)
            {
                FIXME( "not enough room %u/%u for fake . and .. entries\n", length, single_entry );
                fake_dot_dot = 0;
            }
        }

        if (fake_dot_dot)
        {
            if ((info = append_entry( buffer, &io->Information, length, ".", NULL, mask )))
                last_info = info;
            if ((info = append_entry( buffer, &io->Information, length, "..", NULL, mask )))
                last_info = info;

            /* check if we still have enough space for the largest possible entry */
            if (last_info && io->Information + max_dir_info_size > length)
            {
                lseek( fd, 0, SEEK_SET );  /* reset pos to first entry */
                res = 0;
            }
        }
    }

    while (res > 0)
    {
        res -= de->d_reclen;
        if (!(fake_dot_dot && (!strcmp( de->d_name, "." ) || !strcmp( de->d_name, ".." ))) &&
            (info = append_entry( buffer, &io->Information, length, de->d_name, NULL, mask )))
        {
            last_info = info;
            if ((char *)info->FileName + info->FileNameLength > (char *)buffer + length)
            {
                io->u.Status = STATUS_BUFFER_OVERFLOW;
                lseek( fd, old_pos, SEEK_SET );  /* restore pos to previous entry */
                break;
            }
            /* check if we still have enough space for the largest possible entry */
            if (single_entry || io->Information + max_dir_info_size > length)
            {
                if (res > 0) lseek( fd, de->d_off, SEEK_SET );  /* set pos to next entry */
                break;
            }
        }
        old_pos = de->d_off;
        /* move on to the next entry */
        if (res > 0) de = (KERNEL_DIRENT64 *)((char *)de + de->d_reclen);
        else
        {
            res = getdents64( fd, data, size );
            de = (KERNEL_DIRENT64 *)data;
        }
    }

    if (last_info) last_info->NextEntryOffset = 0;
    else io->u.Status = restart_scan ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES;
    res = 0;
done:
    if (data != local_buffer) RtlFreeHeap( GetProcessHeap(), 0, data );
    return res;
}

#elif defined HAVE_GETDIRENTRIES

/***********************************************************************
 *           read_directory_getdirentries
 *
 * Read a directory using the BSD getdirentries system call; helper for NtQueryDirectoryFile.
 */
static int read_directory_getdirentries( int fd, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                         BOOLEAN single_entry, const UNICODE_STRING *mask,
                                         BOOLEAN restart_scan )
{
    long restart_pos;
    ULONG_PTR restart_info_pos = 0;
    size_t size, initial_size = length;
    int res, fake_dot_dot = 1;
    char *data, local_buffer[8192];
    struct dirent *de;
    FILE_BOTH_DIR_INFORMATION *info, *last_info = NULL, *restart_last_info = NULL;

    size = initial_size;
    data = local_buffer;
    if (size > sizeof(local_buffer) && !(data = RtlAllocateHeap( GetProcessHeap(), 0, size )))
    {
        io->u.Status = STATUS_NO_MEMORY;
        return io->u.Status;
    }

    if (restart_scan) lseek( fd, 0, SEEK_SET );

    io->u.Status = STATUS_SUCCESS;

    /* FIXME: should make sure size is larger than filesystem block size */
    res = getdirentries( fd, data, size, &restart_pos );
    if (res == -1)
    {
        io->u.Status = FILE_GetNtStatus();
        res = 0;
        goto done;
    }

    de = (struct dirent *)data;

    if (restart_scan)
    {
        /* check if we got . and .. from getdirentries */
        if (res > 0)
        {
            if (!strcmp( de->d_name, "." ) && res > de->d_reclen)
            {
                struct dirent *next_de = (struct dirent *)(data + de->d_reclen);
                if (!strcmp( next_de->d_name, ".." )) fake_dot_dot = 0;
            }
        }
        /* make sure we have enough room for both entries */
        if (fake_dot_dot)
        {
            static const ULONG min_info_size = (FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName[1]) +
                                                FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName[2]) + 3) & ~3;
            if (length < min_info_size || single_entry)
            {
                FIXME( "not enough room %u/%u for fake . and .. entries\n", length, single_entry );
                fake_dot_dot = 0;
            }
        }

        if (fake_dot_dot)
        {
            if ((info = append_entry( buffer, &io->Information, length, ".", NULL, mask )))
                last_info = info;
            if ((info = append_entry( buffer, &io->Information, length, "..", NULL, mask )))
                last_info = info;

            restart_last_info = last_info;
            restart_info_pos = io->Information;

            /* check if we still have enough space for the largest possible entry */
            if (last_info && io->Information + max_dir_info_size > length)
            {
                lseek( fd, 0, SEEK_SET );  /* reset pos to first entry */
                res = 0;
            }
        }
    }

    while (res > 0)
    {
        res -= de->d_reclen;
        if (de->d_fileno &&
            !(fake_dot_dot && (!strcmp( de->d_name, "." ) || !strcmp( de->d_name, ".." ))) &&
            ((info = append_entry( buffer, &io->Information, length, de->d_name, NULL, mask ))))
        {
            last_info = info;
            if ((char *)info->FileName + info->FileNameLength > (char *)buffer + length)
            {
                lseek( fd, (unsigned long)restart_pos, SEEK_SET );
                if (restart_info_pos)  /* if we have a complete read already, return it */
                {
                    io->Information = restart_info_pos;
                    last_info = restart_last_info;
                    break;
                }
                /* otherwise restart from the start with a smaller size */
                size = (char *)de - data;
                if (!size)
                {
                    io->u.Status = STATUS_BUFFER_OVERFLOW;
                    break;
                }
                io->Information = 0;
                last_info = NULL;
                goto restart;
            }
            /* if we have to return but the buffer contains more data, restart with a smaller size */
            if (res > 0 && (single_entry || io->Information + max_dir_info_size > length))
            {
                lseek( fd, (unsigned long)restart_pos, SEEK_SET );
                size = (char *)de - data;
                io->Information = restart_info_pos;
                last_info = restart_last_info;
                goto restart;
            }
        }
        /* move on to the next entry */
        if (res > 0)
        {
            de = (struct dirent *)((char *)de + de->d_reclen);
            continue;
        }
        if (size < initial_size) break;  /* already restarted once, give up now */
        size = min( size, length - io->Information );
        /* if size is too small don't bother to continue */
        if (size < max_dir_info_size && last_info) break;
        restart_last_info = last_info;
        restart_info_pos = io->Information;
    restart:
        res = getdirentries( fd, data, size, &restart_pos );
        de = (struct dirent *)data;
    }

    if (last_info) last_info->NextEntryOffset = 0;
    else io->u.Status = restart_scan ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES;
    res = 0;
done:
    if (data != local_buffer) RtlFreeHeap( GetProcessHeap(), 0, data );
    return res;
}
#endif  /* HAVE_GETDIRENTRIES */


/***********************************************************************
 *           read_directory_readdir
 *
 * Read a directory using the POSIX readdir interface; helper for NtQueryDirectoryFile.
 */
static void read_directory_readdir( int fd, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                    BOOLEAN single_entry, const UNICODE_STRING *mask,
                                    BOOLEAN restart_scan )
{
    DIR *dir;
    off_t i, old_pos = 0;
    struct dirent *de;
    FILE_BOTH_DIR_INFORMATION *info, *last_info = NULL;

    if (!(dir = opendir( "." )))
    {
        io->u.Status = FILE_GetNtStatus();
        return;
    }

    if (!restart_scan)
    {
        old_pos = lseek( fd, 0, SEEK_CUR );
        /* skip the right number of entries */
        for (i = 0; i < old_pos - 2; i++)
        {
            if (!readdir( dir ))
            {
                closedir( dir );
                io->u.Status = STATUS_NO_MORE_FILES;
                return;
            }
        }
    }
    io->u.Status = STATUS_SUCCESS;

    for (;;)
    {
        if (old_pos == 0)
            info = append_entry( buffer, &io->Information, length, ".", NULL, mask );
        else if (old_pos == 1)
            info = append_entry( buffer, &io->Information, length, "..", NULL, mask );
        else if ((de = readdir( dir )))
        {
            if (strcmp( de->d_name, "." ) && strcmp( de->d_name, ".." ))
                info = append_entry( buffer, &io->Information, length, de->d_name, NULL, mask );
            else
                info = NULL;
        }
        else
            break;
        old_pos++;
        if (info)
        {
            last_info = info;
            if ((char *)info->FileName + info->FileNameLength > (char *)buffer + length)
            {
                io->u.Status = STATUS_BUFFER_OVERFLOW;
                old_pos--;  /* restore pos to previous entry */
                break;
            }
            if (single_entry) break;
            /* check if we still have enough space for the largest possible entry */
            if (io->Information + max_dir_info_size > length) break;
        }
    }

    lseek( fd, old_pos, SEEK_SET );  /* store dir offset as filepos for fd */
    closedir( dir );

    if (last_info) last_info->NextEntryOffset = 0;
    else io->u.Status = restart_scan ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES;
}

/***********************************************************************
 *           read_directory_stat
 *
 * Read a single file from a directory by determining whether the file
 * identified by mask exists using stat.
 */
static int read_directory_stat( int fd, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                BOOLEAN single_entry, const UNICODE_STRING *mask,
                                BOOLEAN restart_scan )
{
    int unix_len, ret, used_default;
    char *unix_name;
    struct stat st;

    TRACE("trying optimisation for file %s\n", debugstr_us( mask ));

    unix_len = ntdll_wcstoumbs( 0, mask->Buffer, mask->Length / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (!(unix_name = RtlAllocateHeap( GetProcessHeap(), 0, unix_len + 1)))
    {
        io->u.Status = STATUS_NO_MEMORY;
        return 0;
    }
    ret = ntdll_wcstoumbs( 0, mask->Buffer, mask->Length / sizeof(WCHAR), unix_name, unix_len,
                           NULL, &used_default );
    if (ret > 0 && !used_default)
    {
        unix_name[ret] = 0;
        if (restart_scan)
        {
            lseek( fd, 0, SEEK_SET );
        }
        else if (lseek( fd, 0, SEEK_CUR ) != 0)
        {
            io->u.Status = STATUS_NO_MORE_FILES;
            ret = 0;
            goto done;
        }

        ret = stat( unix_name, &st );
        if (!ret)
        {
            FILE_BOTH_DIR_INFORMATION *info = append_entry( buffer, &io->Information, length, unix_name, NULL, mask );
            if (info)
            {
                info->NextEntryOffset = 0;
                if ((char *)info->FileName + info->FileNameLength > (char *)buffer + length)
                    io->u.Status = STATUS_BUFFER_OVERFLOW;
                else
                    lseek( fd, 1, SEEK_CUR );
            }
        }
    }
    else ret = -1;

done:
    RtlFreeHeap( GetProcessHeap(), 0, unix_name );

    TRACE("returning %d\n", ret);

    return ret;
}


static inline WCHAR *mempbrkW( const WCHAR *ptr, const WCHAR *accept, size_t n )
{
    const WCHAR *end;
    for (end = ptr + n; ptr < end; ptr++) if (strchrW( accept, *ptr )) return (WCHAR *)ptr;
    return NULL;
}

/******************************************************************************
 *  NtQueryDirectoryFile	[NTDLL.@]
 *  ZwQueryDirectoryFile	[NTDLL.@]
 */
NTSTATUS WINAPI NtQueryDirectoryFile( HANDLE handle, HANDLE event,
                                      PIO_APC_ROUTINE apc_routine, PVOID apc_context,
                                      PIO_STATUS_BLOCK io,
                                      PVOID buffer, ULONG length,
                                      FILE_INFORMATION_CLASS info_class,
                                      BOOLEAN single_entry,
                                      PUNICODE_STRING mask,
                                      BOOLEAN restart_scan )
{
    int cwd, fd, needs_close;
    static const WCHAR wszWildcards[] = { '*','?',0 };

    TRACE("(%p %p %p %p %p %p 0x%08x 0x%08x 0x%08x %s 0x%08x\n",
          handle, event, apc_routine, apc_context, io, buffer,
          length, info_class, single_entry, debugstr_us(mask),
          restart_scan);

    if (length < sizeof(FILE_BOTH_DIR_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;

    if (event || apc_routine)
    {
        FIXME( "Unsupported yet option\n" );
        return io->u.Status = STATUS_NOT_IMPLEMENTED;
    }
    if (info_class != FileBothDirectoryInformation)
    {
        FIXME( "Unsupported file info class %d\n", info_class );
        return io->u.Status = STATUS_NOT_IMPLEMENTED;
    }

    if ((io->u.Status = server_get_unix_fd( handle, FILE_LIST_DIRECTORY, &fd, &needs_close, NULL, NULL )) != STATUS_SUCCESS)
        return io->u.Status;

    io->Information = 0;

    RtlEnterCriticalSection( &dir_section );

    if (show_dot_files == -1) init_options();

    if ((cwd = open(".", O_RDONLY)) != -1 && fchdir( fd ) != -1)
    {
#ifdef VFAT_IOCTL_READDIR_BOTH
        if ((read_directory_vfat( fd, io, buffer, length, single_entry, mask, restart_scan )) != -1)
            goto done;
#endif
        if (mask && !mempbrkW( mask->Buffer, wszWildcards, mask->Length / sizeof(WCHAR) ) &&
            read_directory_stat( fd, io, buffer, length, single_entry, mask, restart_scan ) != -1)
            goto done;
#ifdef USE_GETDENTS
        if ((read_directory_getdents( fd, io, buffer, length, single_entry, mask, restart_scan )) != -1)
            goto done;
#elif defined HAVE_GETDIRENTRIES
        if ((read_directory_getdirentries( fd, io, buffer, length, single_entry, mask, restart_scan )) != -1)
            goto done;
#endif
        read_directory_readdir( fd, io, buffer, length, single_entry, mask, restart_scan );

    done:
        if (fchdir( cwd ) == -1) chdir( "/" );
    }
    else io->u.Status = FILE_GetNtStatus();

    RtlLeaveCriticalSection( &dir_section );

    if (needs_close) close( fd );
    if (cwd != -1) close( cwd );
    TRACE( "=> %x (%ld)\n", io->u.Status, io->Information );
    return io->u.Status;
}


/***********************************************************************
 *           find_file_in_dir
 *
 * Find a file in a directory the hard way, by doing a case-insensitive search.
 * The file found is appended to unix_name at pos.
 * There must be at least MAX_DIR_ENTRY_LEN+2 chars available at pos.
 */
static NTSTATUS find_file_in_dir( char *unix_name, int pos, const WCHAR *name, int length,
                                  int check_case )
{
    WCHAR buffer[MAX_DIR_ENTRY_LEN];
    UNICODE_STRING str;
    BOOLEAN spaces;
    DIR *dir;
    struct dirent *de;
    struct stat st;
    int ret, used_default, is_name_8_dot_3;

    /* try a shortcut for this directory */

    unix_name[pos++] = '/';
    ret = ntdll_wcstoumbs( 0, name, length, unix_name + pos, MAX_DIR_ENTRY_LEN,
                           NULL, &used_default );
    /* if we used the default char, the Unix name won't round trip properly back to Unicode */
    /* so it cannot match the file we are looking for */
    if (ret >= 0 && !used_default)
    {
        unix_name[pos + ret] = 0;
        if (!stat( unix_name, &st )) return STATUS_SUCCESS;
    }
    if (check_case) goto not_found;  /* we want an exact match */

    if (pos > 1) unix_name[pos - 1] = 0;
    else unix_name[1] = 0;  /* keep the initial slash */

    /* check if it fits in 8.3 so that we don't look for short names if we won't need them */

    str.Buffer = (WCHAR *)name;
    str.Length = length * sizeof(WCHAR);
    str.MaximumLength = str.Length;
    is_name_8_dot_3 = RtlIsNameLegalDOS8Dot3( &str, NULL, &spaces ) && !spaces;

    /* now look for it through the directory */

#ifdef VFAT_IOCTL_READDIR_BOTH
    if (is_name_8_dot_3)
    {
        int fd = open( unix_name, O_RDONLY | O_DIRECTORY );
        if (fd != -1)
        {
            KERNEL_DIRENT *de;

            RtlEnterCriticalSection( &dir_section );
            if ((de = start_vfat_ioctl( fd )))
            {
                unix_name[pos - 1] = '/';
                while (de[0].d_reclen)
                {
                    /* make sure names are null-terminated to work around an x86-64 kernel bug */
                    size_t len = min(de[0].d_reclen, sizeof(de[0].d_name) - 1 );
                    de[0].d_name[len] = 0;
                    len = min(de[1].d_reclen, sizeof(de[1].d_name) - 1 );
                    de[1].d_name[len] = 0;

                    if (de[1].d_name[0])
                    {
                        ret = ntdll_umbstowcs( 0, de[1].d_name, strlen(de[1].d_name),
                                               buffer, MAX_DIR_ENTRY_LEN );
                        if (ret == length && !memicmpW( buffer, name, length))
                        {
                            strcpy( unix_name + pos, de[1].d_name );
                            RtlLeaveCriticalSection( &dir_section );
                            close( fd );
                            return STATUS_SUCCESS;
                        }
                    }
                    ret = ntdll_umbstowcs( 0, de[0].d_name, strlen(de[0].d_name),
                                           buffer, MAX_DIR_ENTRY_LEN );
                    if (ret == length && !memicmpW( buffer, name, length))
                    {
                        strcpy( unix_name + pos,
                                de[1].d_name[0] ? de[1].d_name : de[0].d_name );
                        RtlLeaveCriticalSection( &dir_section );
                        close( fd );
                        return STATUS_SUCCESS;
                    }
                    if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) == -1)
                    {
                        RtlLeaveCriticalSection( &dir_section );
                        close( fd );
                        goto not_found;
                    }
                }
            }
            RtlLeaveCriticalSection( &dir_section );
            close( fd );
        }
        /* fall through to normal handling */
    }
#endif /* VFAT_IOCTL_READDIR_BOTH */

    if (!(dir = opendir( unix_name )))
    {
        if (errno == ENOENT) return STATUS_OBJECT_PATH_NOT_FOUND;
        else return FILE_GetNtStatus();
    }
    unix_name[pos - 1] = '/';
    str.Buffer = buffer;
    str.MaximumLength = sizeof(buffer);
    while ((de = readdir( dir )))
    {
        ret = ntdll_umbstowcs( 0, de->d_name, strlen(de->d_name), buffer, MAX_DIR_ENTRY_LEN );
        if (ret == length && !memicmpW( buffer, name, length ))
        {
            strcpy( unix_name + pos, de->d_name );
            closedir( dir );
            return STATUS_SUCCESS;
        }

        if (!is_name_8_dot_3) continue;

        str.Length = ret * sizeof(WCHAR);
        if (!RtlIsNameLegalDOS8Dot3( &str, NULL, &spaces ) || spaces)
        {
            WCHAR short_nameW[12];
            ret = hash_short_file_name( &str, short_nameW );
            if (ret == length && !memicmpW( short_nameW, name, length ))
            {
                strcpy( unix_name + pos, de->d_name );
                closedir( dir );
                return STATUS_SUCCESS;
            }
        }
    }
    closedir( dir );
    goto not_found;  /* avoid warning */

not_found:
    unix_name[pos - 1] = 0;
    return STATUS_OBJECT_PATH_NOT_FOUND;
}


/******************************************************************************
 *           get_dos_device
 *
 * Get the Unix path of a DOS device.
 */
static NTSTATUS get_dos_device( const WCHAR *name, UINT name_len, ANSI_STRING *unix_name_ret )
{
    const char *config_dir = wine_get_config_dir();
    struct stat st;
    char *unix_name, *new_name, *dev;
    unsigned int i;
    int unix_len;

    /* make sure the device name is ASCII */
    for (i = 0; i < name_len; i++)
        if (name[i] <= 32 || name[i] >= 127) return STATUS_BAD_DEVICE_TYPE;

    unix_len = strlen(config_dir) + sizeof("/dosdevices/") + name_len + 1;

    if (!(unix_name = RtlAllocateHeap( GetProcessHeap(), 0, unix_len )))
        return STATUS_NO_MEMORY;

    strcpy( unix_name, config_dir );
    strcat( unix_name, "/dosdevices/" );
    dev = unix_name + strlen(unix_name);

    for (i = 0; i < name_len; i++) dev[i] = (char)tolowerW(name[i]);
    dev[i] = 0;

    /* special case for drive devices */
    if (name_len == 2 && dev[1] == ':')
    {
        dev[i++] = ':';
        dev[i] = 0;
    }

    for (;;)
    {
        if (!stat( unix_name, &st ))
        {
            TRACE( "%s -> %s\n", debugstr_wn(name,name_len), debugstr_a(unix_name) );
            unix_name_ret->Buffer = unix_name;
            unix_name_ret->Length = strlen(unix_name);
            unix_name_ret->MaximumLength = unix_len;
            return STATUS_SUCCESS;
        }
        if (!dev) break;

        /* now try some defaults for it */
        if (!strcmp( dev, "aux" ))
        {
            strcpy( dev, "com1" );
            continue;
        }
        if (!strcmp( dev, "prn" ))
        {
            strcpy( dev, "lpt1" );
            continue;
        }
        if (!strcmp( dev, "nul" ))
        {
            strcpy( unix_name, "/dev/null" );
            dev = NULL; /* last try */
            continue;
        }

        new_name = NULL;
        if (dev[1] == ':' && dev[2] == ':')  /* drive device */
        {
            dev[2] = 0;  /* remove last ':' to get the drive mount point symlink */
            new_name = get_default_drive_device( unix_name );
        }
        else if (!strncmp( dev, "com", 3 )) new_name = get_default_com_device( dev[3] - '0' );
        else if (!strncmp( dev, "lpt", 3 )) new_name = get_default_lpt_device( dev[3] - '0' );

        if (!new_name) break;

        RtlFreeHeap( GetProcessHeap(), 0, unix_name );
        unix_name = new_name;
        unix_len = strlen(unix_name) + 1;
        dev = NULL; /* last try */
    }
    RtlFreeHeap( GetProcessHeap(), 0, unix_name );
    return STATUS_BAD_DEVICE_TYPE;
}


/* return the length of the DOS namespace prefix if any */
static inline int get_dos_prefix_len( const UNICODE_STRING *name )
{
    static const WCHAR nt_prefixW[] = {'\\','?','?','\\'};
    static const WCHAR dosdev_prefixW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\'};

    if (name->Length > sizeof(nt_prefixW) &&
        !memcmp( name->Buffer, nt_prefixW, sizeof(nt_prefixW) ))
        return sizeof(nt_prefixW) / sizeof(WCHAR);

    if (name->Length > sizeof(dosdev_prefixW) &&
        !memicmpW( name->Buffer, dosdev_prefixW, sizeof(dosdev_prefixW)/sizeof(WCHAR) ))
        return sizeof(dosdev_prefixW) / sizeof(WCHAR);

    return 0;
}


/******************************************************************************
 *           wine_nt_to_unix_file_name  (NTDLL.@) Not a Windows API
 *
 * Convert a file name from NT namespace to Unix namespace.
 *
 * If disposition is not FILE_OPEN or FILE_OVERWRITTE, the last path
 * element doesn't have to exist; in that case STATUS_NO_SUCH_FILE is
 * returned, but the unix name is still filled in properly.
 */
NTSTATUS wine_nt_to_unix_file_name( const UNICODE_STRING *nameW, ANSI_STRING *unix_name_ret,
                                    UINT disposition, BOOLEAN check_case )
{
    static const WCHAR unixW[] = {'u','n','i','x'};
    static const WCHAR invalid_charsW[] = { INVALID_NT_CHARS, 0 };

    NTSTATUS status = STATUS_SUCCESS;
    const char *config_dir = wine_get_config_dir();
    const WCHAR *name, *p;
    struct stat st;
    char *unix_name;
    int pos, ret, name_len, unix_len, prefix_len, used_default;
    WCHAR prefix[MAX_DIR_ENTRY_LEN];
    BOOLEAN is_unix = FALSE;

    name     = nameW->Buffer;
    name_len = nameW->Length / sizeof(WCHAR);

    if (!name_len || !IS_SEPARATOR(name[0])) return STATUS_OBJECT_PATH_SYNTAX_BAD;

    if (!(pos = get_dos_prefix_len( nameW )))
        return STATUS_BAD_DEVICE_TYPE;  /* no DOS prefix, assume NT native name */

    name += pos;
    name_len -= pos;

    /* check for sub-directory */
    for (pos = 0; pos < name_len; pos++)
    {
        if (IS_SEPARATOR(name[pos])) break;
        if (name[pos] < 32 || strchrW( invalid_charsW, name[pos] ))
            return STATUS_OBJECT_NAME_INVALID;
    }
    if (pos > MAX_DIR_ENTRY_LEN)
        return STATUS_OBJECT_NAME_INVALID;

    if (pos == name_len)  /* no subdir, plain DOS device */
        return get_dos_device( name, name_len, unix_name_ret );

    for (prefix_len = 0; prefix_len < pos; prefix_len++)
        prefix[prefix_len] = tolowerW(name[prefix_len]);

    name += prefix_len;
    name_len -= prefix_len;

    /* check for invalid characters (all chars except 0 are valid for unix) */
    is_unix = (prefix_len == 4 && !memcmp( prefix, unixW, sizeof(unixW) ));
    if (is_unix)
    {
        for (p = name; p < name + name_len; p++)
            if (!*p) return STATUS_OBJECT_NAME_INVALID;
        check_case = TRUE;
    }
    else
    {
        for (p = name; p < name + name_len; p++)
            if (*p < 32 || strchrW( invalid_charsW, *p )) return STATUS_OBJECT_NAME_INVALID;
    }

    unix_len = ntdll_wcstoumbs( 0, prefix, prefix_len, NULL, 0, NULL, NULL );
    unix_len += ntdll_wcstoumbs( 0, name, name_len, NULL, 0, NULL, NULL );
    unix_len += MAX_DIR_ENTRY_LEN + 3;
    unix_len += strlen(config_dir) + sizeof("/dosdevices/");
    if (!(unix_name = RtlAllocateHeap( GetProcessHeap(), 0, unix_len )))
        return STATUS_NO_MEMORY;
    strcpy( unix_name, config_dir );
    strcat( unix_name, "/dosdevices/" );
    pos = strlen(unix_name);

    ret = ntdll_wcstoumbs( 0, prefix, prefix_len, unix_name + pos, unix_len - pos - 1,
                           NULL, &used_default );
    if (!ret || used_default)
    {
        RtlFreeHeap( GetProcessHeap(), 0, unix_name );
        return STATUS_OBJECT_NAME_INVALID;
    }
    pos += ret;

    /* check if prefix exists (except for DOS drives to avoid extra stat calls) */

    if (prefix_len != 2 || prefix[1] != ':')
    {
        unix_name[pos] = 0;
        if (lstat( unix_name, &st ) == -1 && errno == ENOENT)
        {
            if (!is_unix)
            {
                RtlFreeHeap( GetProcessHeap(), 0, unix_name );
                return STATUS_BAD_DEVICE_TYPE;
            }
            pos = 0;  /* fall back to unix root */
        }
    }

    /* try a shortcut first */

    ret = ntdll_wcstoumbs( 0, name, name_len, unix_name + pos, unix_len - pos - 1,
                           NULL, &used_default );

    while (name_len && IS_SEPARATOR(*name))
    {
        name++;
        name_len--;
    }

    if (ret > 0 && !used_default)  /* if we used the default char the name didn't convert properly */
    {
        char *p;
        unix_name[pos + ret] = 0;
        for (p = unix_name + pos ; *p; p++) if (*p == '\\') *p = '/';
        if (!stat( unix_name, &st ))
        {
            /* creation fails with STATUS_ACCESS_DENIED for the root of the drive */
            if (disposition == FILE_CREATE)
            {
                RtlFreeHeap( GetProcessHeap(), 0, unix_name );
                return name_len ? STATUS_OBJECT_NAME_COLLISION : STATUS_ACCESS_DENIED;
            }
            goto done;
        }
    }

    if (!name_len)  /* empty name -> drive root doesn't exist */
    {
        RtlFreeHeap( GetProcessHeap(), 0, unix_name );
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }
    if (check_case && (disposition == FILE_OPEN || disposition == FILE_OVERWRITE))
    {
        RtlFreeHeap( GetProcessHeap(), 0, unix_name );
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* now do it component by component */

    while (name_len)
    {
        const WCHAR *end, *next;

        end = name;
        while (end < name + name_len && !IS_SEPARATOR(*end)) end++;
        next = end;
        while (next < name + name_len && IS_SEPARATOR(*next)) next++;
        name_len -= next - name;

        /* grow the buffer if needed */

        if (unix_len - pos < MAX_DIR_ENTRY_LEN + 2)
        {
            char *new_name;
            unix_len += 2 * MAX_DIR_ENTRY_LEN;
            if (!(new_name = RtlReAllocateHeap( GetProcessHeap(), 0, unix_name, unix_len )))
            {
                RtlFreeHeap( GetProcessHeap(), 0, unix_name );
                return STATUS_NO_MEMORY;
            }
            unix_name = new_name;
        }

        status = find_file_in_dir( unix_name, pos, name, end - name, check_case );

        /* if this is the last element, not finding it is not necessarily fatal */
        if (!name_len)
        {
            if (status == STATUS_OBJECT_PATH_NOT_FOUND)
            {
                status = STATUS_OBJECT_NAME_NOT_FOUND;
                if (disposition != FILE_OPEN && disposition != FILE_OVERWRITE)
                {
                    ret = ntdll_wcstoumbs( 0, name, end - name, unix_name + pos + 1,
                                           MAX_DIR_ENTRY_LEN, NULL, &used_default );
                    if (ret > 0 && !used_default)
                    {
                        unix_name[pos] = '/';
                        unix_name[pos + 1 + ret] = 0;
                        status = STATUS_NO_SUCH_FILE;
                        break;
                    }
                }
            }
            else if (status == STATUS_SUCCESS && disposition == FILE_CREATE)
            {
                status = STATUS_OBJECT_NAME_COLLISION;
            }
        }

        if (status != STATUS_SUCCESS)
        {
            /* couldn't find it at all, fail */
            WARN( "%s not found in %s\n", debugstr_w(name), unix_name );
            RtlFreeHeap( GetProcessHeap(), 0, unix_name );
            return status;
        }

        pos += strlen( unix_name + pos );
        name = next;
    }

    WARN( "%s -> %s required a case-insensitive search\n",
          debugstr_us(nameW), debugstr_a(unix_name) );

done:
    TRACE( "%s -> %s\n", debugstr_us(nameW), debugstr_a(unix_name) );
    unix_name_ret->Buffer = unix_name;
    unix_name_ret->Length = strlen(unix_name);
    unix_name_ret->MaximumLength = unix_len;
    return status;
}


/******************************************************************
 *		RtlDoesFileExists_U   (NTDLL.@)
 */
BOOLEAN WINAPI RtlDoesFileExists_U(LPCWSTR file_name)
{
    UNICODE_STRING nt_name;
    ANSI_STRING unix_name;
    BOOLEAN ret;

    if (!RtlDosPathNameToNtPathName_U( file_name, &nt_name, NULL, NULL )) return FALSE;
    ret = (wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, FALSE ) == STATUS_SUCCESS);
    if (ret) RtlFreeAnsiString( &unix_name );
    RtlFreeUnicodeString( &nt_name );
    return ret;
}


/***********************************************************************
 *           DIR_unmount_device
 *
 * Unmount the specified device.
 */
NTSTATUS DIR_unmount_device( HANDLE handle )
{
    NTSTATUS status;
    int unix_fd, needs_close;

    SERVER_START_REQ( unmount_device )
    {
        req->handle = handle;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (status) return status;

    if (!(status = server_get_unix_fd( handle, 0, &unix_fd, &needs_close, NULL, NULL )))
    {
        struct stat st;
        char *mount_point = NULL;

        if (fstat( unix_fd, &st ) == -1 || !is_valid_mounted_device( &st ))
            status = STATUS_INVALID_PARAMETER;
        else
        {
            if ((mount_point = get_device_mount_point( st.st_rdev )))
            {
#ifdef __APPLE__
                static const char umount[] = "diskutil unmount >/dev/null 2>&1 ";
#else
                static const char umount[] = "umount >/dev/null 2>&1 ";
#endif
                char *cmd = RtlAllocateHeap( GetProcessHeap(), 0, strlen(mount_point)+sizeof(umount));
                if (cmd)
                {
                    strcpy( cmd, umount );
                    strcat( cmd, mount_point );
                    system( cmd );
                    RtlFreeHeap( GetProcessHeap(), 0, cmd );
#ifdef linux
                    /* umount will fail to release the loop device since we still have
                       a handle to it, so we release it here */
                    if (major(st.st_rdev) == LOOP_MAJOR) ioctl( unix_fd, 0x4c01 /*LOOP_CLR_FD*/, 0 );
#endif
                }
                RtlFreeHeap( GetProcessHeap(), 0, mount_point );
            }
        }
        if (needs_close) close( unix_fd );
    }
    return status;
}


/******************************************************************************
 *           DIR_get_unix_cwd
 *
 * Retrieve the Unix name of the current directory; helper for wine_unix_to_nt_file_name.
 * Returned value must be freed by caller.
 */
NTSTATUS DIR_get_unix_cwd( char **cwd )
{
    int old_cwd, unix_fd, needs_close;
    CURDIR *curdir;
    HANDLE handle;
    NTSTATUS status;

    RtlAcquirePebLock();

    if (NtCurrentTeb()->Tib.SubSystemTib)  /* FIXME: hack */
        curdir = &((WIN16_SUBSYSTEM_TIB *)NtCurrentTeb()->Tib.SubSystemTib)->curdir;
    else
        curdir = &NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectory;

    if (!(handle = curdir->Handle))
    {
        UNICODE_STRING dirW;
        OBJECT_ATTRIBUTES attr;
        IO_STATUS_BLOCK io;

        if (!RtlDosPathNameToNtPathName_U( curdir->DosPath.Buffer, &dirW, NULL, NULL ))
        {
            status = STATUS_OBJECT_NAME_INVALID;
            goto done;
        }
        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.Attributes = OBJ_CASE_INSENSITIVE;
        attr.ObjectName = &dirW;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;

        status = NtOpenFile( &handle, 0, &attr, &io, 0,
                             FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
        RtlFreeUnicodeString( &dirW );
        if (status != STATUS_SUCCESS) goto done;
    }

    if ((status = server_get_unix_fd( handle, 0, &unix_fd, &needs_close, NULL, NULL )) == STATUS_SUCCESS)
    {
        RtlEnterCriticalSection( &dir_section );

        if ((old_cwd = open(".", O_RDONLY)) != -1 && fchdir( unix_fd ) != -1)
        {
            unsigned int size = 512;

            for (;;)
            {
                if (!(*cwd = RtlAllocateHeap( GetProcessHeap(), 0, size )))
                {
                    status = STATUS_NO_MEMORY;
                    break;
                }
                if (getcwd( *cwd, size )) break;
                RtlFreeHeap( GetProcessHeap(), 0, *cwd );
                if (errno != ERANGE)
                {
                    status = STATUS_OBJECT_PATH_INVALID;
                    break;
                }
                size *= 2;
            }
            if (fchdir( old_cwd ) == -1) chdir( "/" );
        }
        else status = FILE_GetNtStatus();

        RtlLeaveCriticalSection( &dir_section );
        if (needs_close) close( unix_fd );
    }
    if (!curdir->Handle) NtClose( handle );

done:
    RtlReleasePebLock();
    return status;
}

struct read_changes_info
{
    HANDLE FileHandle;
    PVOID Buffer;
    ULONG BufferSize;
};

static void WINAPI read_changes_apc( void *user, PIO_STATUS_BLOCK iosb, ULONG status )
{
    struct read_changes_info *info = user;
    char path[PATH_MAX];
    NTSTATUS ret = STATUS_SUCCESS;
    int len, action, i;

    TRACE("%p %p %08x\n", info, iosb, status);

    /*
     * FIXME: race me!
     *
     * hEvent/hDir is set before the output buffer and iosb is updated.
     * Since the thread that called NtNotifyChangeDirectoryFile is usually
     * waiting, we'll be safe since we're called in that thread's context.
     * If a different thread is waiting on our hEvent/hDir we're going to be
     * in trouble...
     */
    SERVER_START_REQ( read_change )
    {
        req->handle = info->FileHandle;
        wine_server_set_reply( req, path, PATH_MAX );
        ret = wine_server_call( req );
        action = reply->action;
        len = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;

    if (ret == STATUS_SUCCESS && info->Buffer && 
        (info->BufferSize > (sizeof (FILE_NOTIFY_INFORMATION) + len*sizeof(WCHAR))))
    {
        PFILE_NOTIFY_INFORMATION pfni;

        pfni = (PFILE_NOTIFY_INFORMATION) info->Buffer;

        /* convert to an NT style path */
        for (i=0; i<len; i++)
            if (path[i] == '/')
                path[i] = '\\';

        len = ntdll_umbstowcs( 0, path, len, pfni->FileName,
                               info->BufferSize - sizeof (*pfni) );

        pfni->NextEntryOffset = 0;
        pfni->Action = action;
        pfni->FileNameLength = len * sizeof (WCHAR);
        pfni->FileName[len] = 0;

        TRACE("action = %d name = %s\n", pfni->Action,
              debugstr_w(pfni->FileName) );
        len = sizeof (*pfni) - sizeof (DWORD) + pfni->FileNameLength;
    }
    else
    {
        ret = STATUS_NOTIFY_ENUM_DIR;
        len = 0;
    }

    iosb->u.Status = ret;
    iosb->Information = len;

    RtlFreeHeap( GetProcessHeap(), 0, info );
}

#define FILE_NOTIFY_ALL        (  \
 FILE_NOTIFY_CHANGE_FILE_NAME   | \
 FILE_NOTIFY_CHANGE_DIR_NAME    | \
 FILE_NOTIFY_CHANGE_ATTRIBUTES  | \
 FILE_NOTIFY_CHANGE_SIZE        | \
 FILE_NOTIFY_CHANGE_LAST_WRITE  | \
 FILE_NOTIFY_CHANGE_LAST_ACCESS | \
 FILE_NOTIFY_CHANGE_CREATION    | \
 FILE_NOTIFY_CHANGE_SECURITY   )

/******************************************************************************
 *  NtNotifyChangeDirectoryFile [NTDLL.@]
 */
NTSTATUS WINAPI
NtNotifyChangeDirectoryFile( HANDLE FileHandle, HANDLE Event,
        PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext,
        PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
        ULONG BufferSize, ULONG CompletionFilter, BOOLEAN WatchTree )
{
    struct read_changes_info *info;
    NTSTATUS status;

    TRACE("%p %p %p %p %p %p %u %u %d\n",
          FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock,
          Buffer, BufferSize, CompletionFilter, WatchTree );

    if (!IoStatusBlock)
        return STATUS_ACCESS_VIOLATION;

    if (CompletionFilter == 0 || (CompletionFilter & ~FILE_NOTIFY_ALL))
        return STATUS_INVALID_PARAMETER;

    info = RtlAllocateHeap( GetProcessHeap(), 0, sizeof *info );
    if (!info)
        return STATUS_NO_MEMORY;

    info->FileHandle = FileHandle;
    info->Buffer     = Buffer;
    info->BufferSize = BufferSize;

    SERVER_START_REQ( read_directory_changes )
    {
        req->handle     = FileHandle;
        req->filter     = CompletionFilter;
        req->want_data  = (Buffer != NULL);
        req->subtree    = WatchTree;
        req->async.callback = read_changes_apc;
        req->async.iosb     = IoStatusBlock;
        req->async.arg      = info;
        req->async.apc      = ApcRoutine;
        req->async.apc_arg  = ApcContext;
        req->async.event    = Event;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING)
        RtlFreeHeap( GetProcessHeap(), 0, info );

    return status;
}
