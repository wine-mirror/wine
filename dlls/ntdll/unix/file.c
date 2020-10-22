/*
 * NTDLL directory and file functions
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <assert.h>
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
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_ATTR_H
#include <sys/attr.h>
#endif
#ifdef MAJOR_IN_MKDEV
# include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
# include <sys/sysmacros.h>
#endif
#ifdef HAVE_SYS_VNODE_H
# ifdef HAVE_STDINT_H
# include <stdint.h>  /* needed for kfreebsd */
# endif
/* Work around a conflict with Solaris' system list defined in sys/list.h. */
#define list SYSLIST
#define list_next SYSLIST_NEXT
#define list_prev SYSLIST_PREV
#define list_head SYSLIST_HEAD
#define list_tail SYSLIST_TAIL
#define list_move_tail SYSLIST_MOVE_TAIL
#define list_remove SYSLIST_REMOVE
#include <sys/vnode.h>
#undef list
#undef list_next
#undef list_prev
#undef list_head
#undef list_tail
#undef list_move_tail
#undef list_remove
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
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winnt.h"
#include "winioctl.h"
#include "winternl.h"
#include "ddk/ntddk.h"
#include "ddk/ntddser.h"
#include "ddk/wdm.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define MAX_DOS_DRIVES 26

#define FILE_WRITE_TO_END_OF_FILE      ((LONGLONG)-1)
#define FILE_USE_FILE_POINTER_POSITION ((LONGLONG)-2)

/* just in case... */
#undef VFAT_IOCTL_READDIR_BOTH
#undef EXT2_IOC_GETFLAGS
#undef EXT4_CASEFOLD_FL

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

/* Define the ext2 ioctl for handling extra attributes */
#define EXT2_IOC_GETFLAGS _IOR('f', 1, long)

/* Case-insensitivity attribute */
#define EXT4_CASEFOLD_FL 0x40000000

#ifndef O_DIRECTORY
# define O_DIRECTORY 0200000 /* must be directory */
#endif

#ifndef AT_NO_AUTOMOUNT
#define AT_NO_AUTOMOUNT 0x800
#endif

#endif  /* linux */

#define IS_SEPARATOR(ch)   ((ch) == '\\' || (ch) == '/')

#define INVALID_NT_CHARS   '*','?','<','>','|','"'
#define INVALID_DOS_CHARS  INVALID_NT_CHARS,'+','=',',',';','[',']',' ','\345'

#define MAX_DIR_ENTRY_LEN 255  /* max length of a directory entry in chars */

#define MAX_IGNORED_FILES 4

struct file_identity
{
    dev_t dev;
    ino_t ino;
};

static struct file_identity ignored_files[MAX_IGNORED_FILES];
static unsigned int ignored_files_count;

union file_directory_info
{
    ULONG                              next;
    FILE_DIRECTORY_INFORMATION         dir;
    FILE_BOTH_DIRECTORY_INFORMATION    both;
    FILE_FULL_DIRECTORY_INFORMATION    full;
    FILE_ID_BOTH_DIRECTORY_INFORMATION id_both;
    FILE_ID_FULL_DIRECTORY_INFORMATION id_full;
    FILE_ID_GLOBAL_TX_DIR_INFORMATION  id_tx;
    FILE_NAMES_INFORMATION             names;
};

struct dir_data_buffer
{
    struct dir_data_buffer *next;    /* next buffer in the list */
    unsigned int            size;    /* total size of the buffer */
    unsigned int            pos;     /* current position in the buffer */
    char                    data[1];
};

struct dir_data_names
{
    const WCHAR *long_name;          /* long file name in Unicode */
    const WCHAR *short_name;         /* short file name in Unicode */
    const char  *unix_name;          /* Unix file name in host encoding */
};

struct dir_data
{
    unsigned int            size;    /* size of the names array */
    unsigned int            count;   /* count of used entries in the names array */
    unsigned int            pos;     /* current reading position in the names array */
    struct file_identity    id;      /* directory file identity */
    struct dir_data_names  *names;   /* directory file names */
    struct dir_data_buffer *buffer;  /* head of data buffers list */
};

static const unsigned int dir_data_buffer_initial_size = 4096;
static const unsigned int dir_data_cache_initial_size  = 256;
static const unsigned int dir_data_names_initial_size  = 64;

static struct dir_data **dir_data_cache;
static unsigned int dir_data_cache_size;

static BOOL show_dot_files;
static mode_t start_umask;

/* at some point we may want to allow Winelib apps to set this */
static const BOOL is_case_sensitive = FALSE;

static struct file_identity windir;

static pthread_mutex_t dir_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mnt_mutex = PTHREAD_MUTEX_INITIALIZER;

/* check if a given Unicode char is OK in a DOS short name */
static inline BOOL is_invalid_dos_char( WCHAR ch )
{
    static const WCHAR invalid_chars[] = { INVALID_DOS_CHARS,'~','.',0 };
    if (ch > 0x7f) return TRUE;
    return wcschr( invalid_chars, ch ) != NULL;
}

/* check if the device can be a mounted volume */
static inline BOOL is_valid_mounted_device( const struct stat *st )
{
#if defined(linux) || defined(__sun__)
    return S_ISBLK( st->st_mode );
#else
    /* disks are char devices on *BSD */
    return S_ISCHR( st->st_mode );
#endif
}

static inline void ignore_file( const char *name )
{
    struct stat st;
    assert( ignored_files_count < MAX_IGNORED_FILES );
    if (!stat( name, &st ))
    {
        ignored_files[ignored_files_count].dev = st.st_dev;
        ignored_files[ignored_files_count].ino = st.st_ino;
        ignored_files_count++;
    }
}

static inline BOOL is_same_file( const struct file_identity *file, const struct stat *st )
{
    return st->st_dev == file->dev && st->st_ino == file->ino;
}

static inline BOOL is_ignored_file( const struct stat *st )
{
    unsigned int i;

    for (i = 0; i < ignored_files_count; i++)
        if (is_same_file( &ignored_files[i], st )) return TRUE;
    return FALSE;
}

static inline unsigned int dir_info_align( unsigned int len )
{
    return (len + 7) & ~7;
}

static inline unsigned int dir_info_size( FILE_INFORMATION_CLASS class, unsigned int len )
{
    switch (class)
    {
    case FileDirectoryInformation:
        return offsetof( FILE_DIRECTORY_INFORMATION, FileName[len] );
    case FileBothDirectoryInformation:
        return offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[len] );
    case FileFullDirectoryInformation:
        return offsetof( FILE_FULL_DIRECTORY_INFORMATION, FileName[len] );
    case FileIdBothDirectoryInformation:
        return offsetof( FILE_ID_BOTH_DIRECTORY_INFORMATION, FileName[len] );
    case FileIdFullDirectoryInformation:
        return offsetof( FILE_ID_FULL_DIRECTORY_INFORMATION, FileName[len] );
    case FileIdGlobalTxDirectoryInformation:
        return offsetof( FILE_ID_GLOBAL_TX_DIR_INFORMATION, FileName[len] );
    case FileNamesInformation:
        return offsetof( FILE_NAMES_INFORMATION, FileName[len] );
    default:
        assert(0);
        return 0;
    }
}

static inline BOOL has_wildcard( const UNICODE_STRING *mask )
{
    int i;

    if (!mask) return TRUE;
    for (i = 0; i < mask->Length / sizeof(WCHAR); i++)
        if (mask->Buffer[i] == '*' || mask->Buffer[i] == '?') return TRUE;
    return FALSE;
}

NTSTATUS errno_to_status( int err )
{
    TRACE( "errno = %d\n", err );
    switch (err)
    {
    case EAGAIN:    return STATUS_SHARING_VIOLATION;
    case EBADF:     return STATUS_INVALID_HANDLE;
    case EBUSY:     return STATUS_DEVICE_BUSY;
    case ENOSPC:    return STATUS_DISK_FULL;
    case EPERM:
    case EROFS:
    case EACCES:    return STATUS_ACCESS_DENIED;
    case ENOTDIR:   return STATUS_OBJECT_PATH_NOT_FOUND;
    case ENOENT:    return STATUS_OBJECT_NAME_NOT_FOUND;
    case EISDIR:    return STATUS_FILE_IS_A_DIRECTORY;
    case EMFILE:
    case ENFILE:    return STATUS_TOO_MANY_OPENED_FILES;
    case EINVAL:    return STATUS_INVALID_PARAMETER;
    case ENOTEMPTY: return STATUS_DIRECTORY_NOT_EMPTY;
    case EPIPE:     return STATUS_PIPE_DISCONNECTED;
    case EIO:       return STATUS_DEVICE_NOT_READY;
#ifdef ENOMEDIUM
    case ENOMEDIUM: return STATUS_NO_MEDIA_IN_DEVICE;
#endif
    case ENXIO:     return STATUS_NO_SUCH_DEVICE;
    case ENOTTY:
    case EOPNOTSUPP:return STATUS_NOT_SUPPORTED;
    case ECONNRESET:return STATUS_PIPE_DISCONNECTED;
    case EFAULT:    return STATUS_ACCESS_VIOLATION;
    case ESPIPE:    return STATUS_ILLEGAL_FUNCTION;
    case ELOOP:     return STATUS_REPARSE_POINT_NOT_RESOLVED;
#ifdef ETIME /* Missing on FreeBSD */
    case ETIME:     return STATUS_IO_TIMEOUT;
#endif
    case ENOEXEC:   /* ?? */
    case EEXIST:    /* ?? */
    default:
        FIXME( "Converting errno %d to STATUS_UNSUCCESSFUL\n", err );
        return STATUS_UNSUCCESSFUL;
    }
}

/* get space from the current directory data buffer, allocating a new one if necessary */
static void *get_dir_data_space( struct dir_data *data, unsigned int size )
{
    struct dir_data_buffer *buffer = data->buffer;
    void *ret;

    if (!buffer || size > buffer->size - buffer->pos)
    {
        unsigned int new_size = buffer ? buffer->size * 2 : dir_data_buffer_initial_size;
        if (new_size < size) new_size = size;
        if (!(buffer = malloc( offsetof( struct dir_data_buffer, data[new_size] ) ))) return NULL;
        buffer->pos  = 0;
        buffer->size = new_size;
        buffer->next = data->buffer;
        data->buffer = buffer;
    }
    ret = buffer->data + buffer->pos;
    buffer->pos += size;
    return ret;
}

/* add a string to the directory data buffer */
static const char *add_dir_data_nameA( struct dir_data *data, const char *name )
{
    /* keep buffer data WCHAR-aligned */
    char *ptr = get_dir_data_space( data, (strlen( name ) + sizeof(WCHAR)) & ~(sizeof(WCHAR) - 1) );
    if (ptr) strcpy( ptr, name );
    return ptr;
}

/* add a Unicode string to the directory data buffer */
static const WCHAR *add_dir_data_nameW( struct dir_data *data, const WCHAR *name )
{
    WCHAR *ptr = get_dir_data_space( data, (wcslen( name ) + 1) * sizeof(WCHAR) );
    if (ptr) wcscpy( ptr, name );
    return ptr;
}

/* add an entry to the directory names array */
static BOOL add_dir_data_names( struct dir_data *data, const WCHAR *long_name,
                                const WCHAR *short_name, const char *unix_name )
{
    static const WCHAR empty[1];
    struct dir_data_names *names = data->names;

    if (data->count >= data->size)
    {
        unsigned int new_size = max( data->size * 2, dir_data_names_initial_size );

        if (!(names = realloc( names, new_size * sizeof(*names) ))) return FALSE;
        data->size  = new_size;
        data->names = names;
    }

    if (short_name[0])
    {
        if (!(names[data->count].short_name = add_dir_data_nameW( data, short_name ))) return FALSE;
    }
    else names[data->count].short_name = empty;

    if (!(names[data->count].long_name = add_dir_data_nameW( data, long_name ))) return FALSE;
    if (!(names[data->count].unix_name = add_dir_data_nameA( data, unix_name ))) return FALSE;
    data->count++;
    return TRUE;
}

/* free the complete directory data structure */
static void free_dir_data( struct dir_data *data )
{
    struct dir_data_buffer *buffer, *next;

    if (!data) return;

    for (buffer = data->buffer; buffer; buffer = next)
    {
        next = buffer->next;
        free( buffer );
    }
    free( data->names );
    free( data );
}


/* support for a directory queue for filesystem searches */

struct dir_name
{
    struct list entry;
    char name[1];
};

static struct list dir_queue = LIST_INIT( dir_queue );

static NTSTATUS add_dir_to_queue( const char *name )
{
    int len = strlen( name ) + 1;
    struct dir_name *dir = malloc( offsetof( struct dir_name, name[len] ));
    if (!dir) return STATUS_NO_MEMORY;
    strcpy( dir->name, name );
    list_add_tail( &dir_queue, &dir->entry );
    return STATUS_SUCCESS;
}

static NTSTATUS next_dir_in_queue( char *name )
{
    struct list *head = list_head( &dir_queue );
    if (head)
    {
        struct dir_name *dir = LIST_ENTRY( head, struct dir_name, entry );
        strcpy( name, dir->name );
        list_remove( &dir->entry );
        free( dir );
        return STATUS_SUCCESS;
    }
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

static void flush_dir_queue(void)
{
    struct list *head;

    while ((head = list_head( &dir_queue )))
    {
        struct dir_name *dir = LIST_ENTRY( head, struct dir_name, entry );
        list_remove( &dir->entry );
        free( dir );
    }
}


#ifdef __ANDROID__

static char *unescape_field( char *str )
{
    char *in, *out;

    for (in = out = str; *in; in++, out++)
    {
        *out = *in;
        if (in[0] == '\\')
        {
            if (in[1] == '\\')
            {
                out[0] = '\\';
                in++;
            }
            else if (in[1] == '0' && in[2] == '4' && in[3] == '0')
            {
                out[0] = ' ';
                in += 3;
            }
            else if (in[1] == '0' && in[2] == '1' && in[3] == '1')
            {
                out[0] = '\t';
                in += 3;
            }
            else if (in[1] == '0' && in[2] == '1' && in[3] == '2')
            {
                out[0] = '\n';
                in += 3;
            }
            else if (in[1] == '1' && in[2] == '3' && in[3] == '4')
            {
                out[0] = '\\';
                in += 3;
            }
        }
    }
    *out = '\0';

    return str;
}

static inline char *get_field( char **str )
{
    char *ret;

    ret = strsep( str, " \t" );
    if (*str) *str += strspn( *str, " \t" );

    return ret;
}
/************************************************************************
 *                    getmntent_replacement
 *
 * getmntent replacement for Android.
 *
 * NB returned static buffer is not thread safe; protect with mnt_mutex.
 */
static struct mntent *getmntent_replacement( FILE *f )
{
    static struct mntent entry;
    static char buf[4096];
    char *p, *start;

    do
    {
        if (!fgets( buf, sizeof(buf), f )) return NULL;
        p = strchr( buf, '\n' );
        if (p) *p = '\0';
        else /* Partially unread line, move file ptr to end */
        {
            char tmp[1024];
            while (fgets( tmp, sizeof(tmp), f ))
                if (strchr( tmp, '\n' )) break;
        }
        start = buf + strspn( buf, " \t" );
    } while (start[0] == '\0' || start[0] == '#');

    p = get_field( &start );
    entry.mnt_fsname = p ? unescape_field( p ) : (char *)"";

    p = get_field( &start );
    entry.mnt_dir = p ? unescape_field( p ) : (char *)"";

    p = get_field( &start );
    entry.mnt_type = p ? unescape_field( p ) : (char *)"";

    p = get_field( &start );
    entry.mnt_opts = p ? unescape_field( p ) : (char *)"";

    p = get_field( &start );
    entry.mnt_freq = p ? atoi(p) : 0;

    p = get_field( &start );
    entry.mnt_passno = p ? atoi(p) : 0;

    return &entry;
}
#define getmntent getmntent_replacement
#endif

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
            !strcmp( entry->mnt_type, "cifs" ) ||
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

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
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

    mutex_lock( &mnt_mutex );

#ifdef __ANDROID__
    if ((f = fopen( "/proc/mounts", "r" )))
    {
        device = parse_mount_entries( f, st.st_dev, st.st_ino );
        fclose( f );
    }
#else
    if ((f = fopen( "/etc/mtab", "r" )))
    {
        device = parse_mount_entries( f, st.st_dev, st.st_ino );
        fclose( f );
    }
    /* look through fstab too in case it's not mounted (for instance if it's an audio CD) */
    if (!device && (f = fopen( "/etc/fstab", "r" )))
    {
        device = parse_mount_entries( f, st.st_dev, st.st_ino );
        fclose( f );
    }
#endif
    if (device) ret = strdup( device );
    mutex_unlock( &mnt_mutex );

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__ ) || defined(__DragonFly__)
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

    mutex_lock( &mnt_mutex );

    /* The FreeBSD parse_mount_entries doesn't require a file argument, so just
     * pass NULL.  Leave the argument in for symmetry.
     */
    device = parse_mount_entries( NULL, st.st_dev, st.st_ino );
    if (device) ret = strdup( device );
    mutex_unlock( &mnt_mutex );

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

    mutex_lock( &mnt_mutex );

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
    if (device) ret = strdup( device );
    mutex_unlock( &mnt_mutex );

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

    mutex_lock( &mnt_mutex );

    mntSize = getmntinfo(&mntStat, MNT_NOWAIT);

    for (i = 0; i < mntSize && !ret; i++)
    {
        if (stat(mntStat[i].f_mntonname, &st ) == -1) continue;
        if (st.st_dev != dev || st.st_ino != ino) continue;

        /* FIXME add support for mounted network drive */
        if ( strncmp(mntStat[i].f_mntfromname, path_bsd_device, strlen(path_bsd_device)) == 0)
        {
            /* set return value to the corresponding raw BSD node */
            ret = malloc( strlen(mntStat[i].f_mntfromname) + 2 /* 2 : r and \0 */ );
            if (ret)
            {
                strcpy(ret, "/dev/r");
                strcat(ret, mntStat[i].f_mntfromname+sizeof("/dev/")-1);
            }
        }
    }
    mutex_unlock( &mnt_mutex );
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

    mutex_lock( &mnt_mutex );

#ifdef __ANDROID__
    if ((f = fopen( "/proc/mounts", "r" )))
#else
    if ((f = fopen( "/etc/mtab", "r" )))
#endif
    {
        struct mntent *entry;
        struct stat st;
        char *p, *device;

        while ((entry = getmntent( f )))
        {
            /* don't even bother stat'ing network mounts, there's no meaningful device anyway */
            if (!strcmp( entry->mnt_type, "nfs" ) ||
                !strcmp( entry->mnt_type, "cifs" ) ||
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
                ret = strdup( entry->mnt_dir );
                break;
            }
        }
        fclose( f );
    }
    mutex_unlock( &mnt_mutex );
#elif defined(__APPLE__)
    struct statfs *entry;
    struct stat st;
    int i, size;

    mutex_lock( &mnt_mutex );

    size = getmntinfo( &entry, MNT_NOWAIT );
    for (i = 0; i < size; i++)
    {
        if (stat( entry[i].f_mntfromname, &st ) == -1) continue;
        if (S_ISBLK(st.st_mode) && st.st_rdev == dev)
        {
            ret = strdup( entry[i].f_mntonname );
            break;
        }
    }
    mutex_unlock( &mnt_mutex );
#else
    static int warned;
    if (!warned++) FIXME( "unmounting devices not supported on this platform\n" );
#endif
    return ret;
}


#if defined(HAVE_GETATTRLIST) && defined(ATTR_VOL_CAPABILITIES) && \
    defined(VOL_CAPABILITIES_FORMAT) && defined(VOL_CAP_FMT_CASE_SENSITIVE)

struct get_fsid
{
    ULONG size;
    dev_t dev;
    fsid_t fsid;
};

struct fs_cache
{
    dev_t dev;
    fsid_t fsid;
    BOOLEAN case_sensitive;
} fs_cache[64];

struct vol_caps
{
    ULONG size;
    vol_capabilities_attr_t caps;
};

/***********************************************************************
 *           look_up_fs_cache
 *
 * Checks if the specified file system is in the cache.
 */
static struct fs_cache *look_up_fs_cache( dev_t dev )
{
    int i;
    for (i = 0; i < ARRAY_SIZE( fs_cache ); i++)
        if (fs_cache[i].dev == dev)
            return fs_cache+i;
    return NULL;
}

/***********************************************************************
 *           add_fs_cache
 *
 * Adds the specified file system to the cache.
 */
static void add_fs_cache( dev_t dev, fsid_t fsid, BOOLEAN case_sensitive )
{
    int i;
    struct fs_cache *entry = look_up_fs_cache( dev );
    static int once = 0;
    if (entry)
    {
        /* Update the cache */
        entry->fsid = fsid;
        entry->case_sensitive = case_sensitive;
        return;
    }

    /* Add a new entry */
    for (i = 0; i < ARRAY_SIZE( fs_cache ); i++)
        if (fs_cache[i].dev == 0)
        {
            /* This entry is empty, use it */
            fs_cache[i].dev = dev;
            fs_cache[i].fsid = fsid;
            fs_cache[i].case_sensitive = case_sensitive;
            return;
        }

    /* Cache is out of space, warn */
    if (!once++)
        WARN( "FS cache is out of space, expect performance problems\n" );
}

/***********************************************************************
 *           get_dir_case_sensitivity_attr
 *
 * Checks if the volume containing the specified directory is case
 * sensitive or not. Uses getattrlist(2).
 */
static int get_dir_case_sensitivity_attr( const char *dir )
{
    char *mntpoint;
    struct attrlist attr;
    struct vol_caps caps;
    struct get_fsid get_fsid;
    struct fs_cache *entry;

    /* First get the FS ID of the volume */
    attr.bitmapcount = ATTR_BIT_MAP_COUNT;
    attr.reserved = 0;
    attr.commonattr = ATTR_CMN_DEVID|ATTR_CMN_FSID;
    attr.volattr = attr.dirattr = attr.fileattr = attr.forkattr = 0;
    get_fsid.size = 0;
    if (getattrlist( dir, &attr, &get_fsid, sizeof(get_fsid), 0 ) != 0 ||
        get_fsid.size != sizeof(get_fsid))
        return -1;
    /* Try to look it up in the cache */
    entry = look_up_fs_cache( get_fsid.dev );
    if (entry && !memcmp( &entry->fsid, &get_fsid.fsid, sizeof(fsid_t) ))
        /* Cache lookup succeeded */
        return entry->case_sensitive;
    /* Cache is stale at this point, we have to update it */

    mntpoint = get_device_mount_point( get_fsid.dev );
    /* Now look up the case-sensitivity */
    attr.commonattr = 0;
    attr.volattr = ATTR_VOL_INFO|ATTR_VOL_CAPABILITIES;
    if (getattrlist( mntpoint, &attr, &caps, sizeof(caps), 0 ) < 0)
    {
        free( mntpoint );
        add_fs_cache( get_fsid.dev, get_fsid.fsid, TRUE );
        return TRUE;
    }
    free( mntpoint );
    if (caps.size == sizeof(caps) &&
        (caps.caps.valid[VOL_CAPABILITIES_FORMAT] &
         (VOL_CAP_FMT_CASE_SENSITIVE | VOL_CAP_FMT_CASE_PRESERVING)) ==
        (VOL_CAP_FMT_CASE_SENSITIVE | VOL_CAP_FMT_CASE_PRESERVING))
    {
        BOOLEAN ret;

        if ((caps.caps.capabilities[VOL_CAPABILITIES_FORMAT] &
            VOL_CAP_FMT_CASE_SENSITIVE) != VOL_CAP_FMT_CASE_SENSITIVE)
            ret = FALSE;
        else
            ret = TRUE;
        /* Update the cache */
        add_fs_cache( get_fsid.dev, get_fsid.fsid, ret );
        return ret;
    }
    return FALSE;
}
#endif

/***********************************************************************
 *           get_dir_case_sensitivity_stat
 *
 * Checks if the volume containing the specified directory is case
 * sensitive or not. Uses (f)statfs(2), statvfs(2), fstatat(2), or ioctl(2).
 */
static BOOLEAN get_dir_case_sensitivity_stat( const char *dir )
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    struct statfs stfs;

    if (statfs( dir, &stfs ) == -1) return FALSE;
    /* Assume these file systems are always case insensitive on Mac OS.
     * For FreeBSD, only assume CIOPFS is case insensitive (AFAIK, Mac OS
     * is the only UNIX that supports case-insensitive lookup).
     */
    if (!strcmp( stfs.f_fstypename, "fusefs" ) &&
        !strncmp( stfs.f_mntfromname, "ciopfs", 5 ))
        return FALSE;
#ifdef __APPLE__
    if (!strcmp( stfs.f_fstypename, "msdos" ) ||
        !strcmp( stfs.f_fstypename, "cd9660" ) ||
        !strcmp( stfs.f_fstypename, "udf" ) ||
        !strcmp( stfs.f_fstypename, "ntfs" ) ||
        !strcmp( stfs.f_fstypename, "smbfs" ))
        return FALSE;
#ifdef _DARWIN_FEATURE_64_BIT_INODE
     if (!strcmp( stfs.f_fstypename, "hfs" ) && (stfs.f_fssubtype == 0 ||
                                                 stfs.f_fssubtype == 1 ||
                                                 stfs.f_fssubtype == 128))
        return FALSE;
#else
     /* The field says "reserved", but a quick look at the kernel source
      * tells us that this "reserved" field is really the same as the
      * "fssubtype" field from the inode64 structure (see munge_statfs()
      * in <xnu-source>/bsd/vfs/vfs_syscalls.c).
      */
     if (!strcmp( stfs.f_fstypename, "hfs" ) && (stfs.f_reserved1 == 0 ||
                                                 stfs.f_reserved1 == 1 ||
                                                 stfs.f_reserved1 == 128))
        return FALSE;
#endif
#endif
    return TRUE;

#elif defined(__NetBSD__)
    struct statvfs stfs;

    if (statvfs( dir, &stfs ) == -1) return FALSE;
    /* Only assume CIOPFS is case insensitive. */
    if (strcmp( stfs.f_fstypename, "fusefs" ) ||
        strncmp( stfs.f_mntfromname, "ciopfs", 5 ))
        return TRUE;
    return FALSE;

#elif defined(__linux__)
    BOOLEAN sens = TRUE;
    struct statfs stfs;
    struct stat st;
    int fd, flags;

    if ((fd = open( dir, O_RDONLY | O_NONBLOCK | O_LARGEFILE )) == -1)
        return TRUE;

    if (ioctl( fd, EXT2_IOC_GETFLAGS, &flags ) != -1 && (flags & EXT4_CASEFOLD_FL))
    {
        sens = FALSE;
    }
    else if (fstatfs( fd, &stfs ) == 0 &&                          /* CIOPFS is case insensitive.  Instead of */
             stfs.f_type == 0x65735546 /* FUSE_SUPER_MAGIC */ &&   /* parsing mtab to discover if the FUSE FS */
             fstatat( fd, ".ciopfs", &st, AT_NO_AUTOMOUNT ) == 0)  /* is CIOPFS, look for .ciopfs in the dir. */
    {
        sens = FALSE;
    }

    close( fd );
    return sens;
#else
    return TRUE;
#endif
}


/***********************************************************************
 *           get_dir_case_sensitivity
 *
 * Checks if the volume containing the specified directory is case
 * sensitive or not. Uses multiple methods, depending on platform.
 */
static BOOLEAN get_dir_case_sensitivity( const char *dir )
{
#if defined(HAVE_GETATTRLIST) && defined(ATTR_VOL_CAPABILITIES) && \
    defined(VOL_CAPABILITIES_FORMAT) && defined(VOL_CAP_FMT_CASE_SENSITIVE)
    int case_sensitive = get_dir_case_sensitivity_attr( dir );
    if (case_sensitive != -1) return case_sensitive;
#endif
    return get_dir_case_sensitivity_stat( dir );
}


/***********************************************************************
 *           is_hidden_file
 *
 * Check if the specified file should be hidden based on its name and the show dot files option.
 */
static BOOL is_hidden_file( const UNICODE_STRING *name )
{
    WCHAR *p, *end;

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
 * Transform a Unix file name into a hashed DOS name. If the name is not a valid
 * DOS name, it is replaced by a hashed version that fits in 8.3 format.
 * 'buffer' must be at least 12 characters long.
 * Returns length of short name in bytes; short name is NOT null-terminated.
 */
static ULONG hash_short_file_name( const WCHAR *name, int length, LPWSTR buffer )
{
    static const char hash_chars[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    LPCWSTR p, ext, end = name + length;
    LPWSTR dst;
    unsigned short hash;
    int i;

    /* Compute the hash code of the file name */
    /* If you know something about hash functions, feel free to */
    /* insert a better algorithm here... */
    if (!is_case_sensitive)
    {
        for (p = name, hash = 0xbeef; p < end - 1; p++)
            hash = (hash<<3) ^ (hash>>5) ^ towlower(*p) ^ (towlower(p[1]) << 8);
        hash = (hash<<3) ^ (hash>>5) ^ towlower(*p); /* Last character */
    }
    else
    {
        for (p = name, hash = 0xbeef; p < end - 1; p++)
            hash = (hash << 3) ^ (hash >> 5) ^ *p ^ (p[1] << 8);
        hash = (hash << 3) ^ (hash >> 5) ^ *p;  /* Last character */
    }

    /* Find last dot for start of the extension */
    for (p = name + 1, ext = NULL; p < end - 1; p++) if (*p == '.') ext = p;

    /* Copy first 4 chars, replacing invalid chars with '_' */
    for (i = 4, p = name, dst = buffer; i > 0; i--, p++)
    {
        if (p == end || p == ext) break;
        *dst++ = is_invalid_dos_char(*p) ? '_' : *p;
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
            *dst++ = is_invalid_dos_char(*ext) ? '_' : *ext;
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
static BOOLEAN match_filename( const WCHAR *name, int length, const UNICODE_STRING *mask_str )
{
    BOOL mismatch;
    const WCHAR *mask = mask_str->Buffer;
    const WCHAR *name_end = name + length;
    const WCHAR *mask_end = mask + mask_str->Length / sizeof(WCHAR);
    const WCHAR *lastjoker = NULL;
    const WCHAR *next_to_retry = NULL;

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
                while (name < name_end && (towupper(*name) != towupper(*mask))) name++;
            next_to_retry = name;
            break;
        case '?':
            mask++;
            name++;
            break;
        default:
            if (is_case_sensitive) mismatch = (*mask != *name);
            else mismatch = (towupper(*mask) != towupper(*name));

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
 *           is_legal_8dot3_name
 *
 * Simplified version of RtlIsNameLegalDOS8Dot3.
 */
static BOOLEAN is_legal_8dot3_name( const WCHAR *name, int len )
{
    static const WCHAR invalid_chars[] = { INVALID_DOS_CHARS,':','/','\\',0 };
    int i, dot = -1;

    if (len > 12) return FALSE;

    /* a starting . is invalid, except for . and .. */
    if (len > 0 && name[0] == '.') return (len == 1 || (len == 2 && name[1] == '.'));

    for (i = 0; i < len; i++)
    {
        if (name[i] > 0x7f) return FALSE;
        if (wcschr( invalid_chars, name[i] )) return FALSE;
        if (name[i] == '.')
        {
            if (dot != -1) return FALSE;
            dot = i;
        }
    }

    if (dot == -1) return (len <= 8);
    if (dot > 8) return FALSE;
    return (len - dot > 1 && len - dot < 5);
}


/***********************************************************************
 *           append_entry
 *
 * Add a file to the directory data if it matches the mask.
 */
static BOOL append_entry( struct dir_data *data, const char *long_name,
                          const char *short_name, const UNICODE_STRING *mask )
{
    int long_len, short_len;
    WCHAR long_nameW[MAX_DIR_ENTRY_LEN + 1];
    WCHAR short_nameW[13];

    long_len = ntdll_umbstowcs( long_name, strlen(long_name), long_nameW, ARRAY_SIZE(long_nameW) );
    if (long_len == ARRAY_SIZE(long_nameW)) return TRUE;
    long_nameW[long_len] = 0;

    if (short_name)
    {
        short_len = ntdll_umbstowcs( short_name, strlen(short_name),
                                     short_nameW, ARRAY_SIZE( short_nameW ) - 1 );
    }
    else  /* generate a short name if necessary */
    {
        short_len = 0;
        if (!is_legal_8dot3_name( long_nameW, long_len ))
            short_len = hash_short_file_name( long_nameW, long_len, short_nameW );
    }
    short_nameW[short_len] = 0;
    wcsupr( short_nameW );

    TRACE( "long %s short %s mask %s\n",
           debugstr_w( long_nameW ), debugstr_w( short_nameW ), debugstr_us( mask ));

    if (mask && !match_filename( long_nameW, long_len, mask ))
    {
        if (!short_len) return TRUE;  /* no short name to match */
        if (!match_filename( short_nameW, short_len, mask )) return TRUE;
    }

    return add_dir_data_names( data, long_nameW, short_nameW, long_name );
}


/* fetch the attributes of a file */
static inline ULONG get_file_attributes( const struct stat *st )
{
    ULONG attr;

    if (S_ISDIR(st->st_mode))
        attr = FILE_ATTRIBUTE_DIRECTORY;
    else
        attr = FILE_ATTRIBUTE_ARCHIVE;
    if (!(st->st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
        attr |= FILE_ATTRIBUTE_READONLY;
    return attr;
}


static BOOL fd_is_mount_point( int fd, const struct stat *st )
{
    struct stat parent;
    return S_ISDIR( st->st_mode ) && !fstatat( fd, "..", &parent, 0 )
            && (parent.st_dev != st->st_dev || parent.st_ino == st->st_ino);
}


/* get the stat info and file attributes for a file (by file descriptor) */
static int fd_get_file_info( int fd, unsigned int options, struct stat *st, ULONG *attr )
{
    int ret;

    *attr = 0;
    ret = fstat( fd, st );
    if (ret == -1) return ret;
    *attr |= get_file_attributes( st );
    /* consider mount points to be reparse points (IO_REPARSE_TAG_MOUNT_POINT) */
    if ((options & FILE_OPEN_REPARSE_POINT) && fd_is_mount_point( fd, st ))
        *attr |= FILE_ATTRIBUTE_REPARSE_POINT;
    return ret;
}


/* get the stat info and file attributes for a file (by name) */
static int get_file_info( const char *path, struct stat *st, ULONG *attr )
{
    char *parent_path;
    int ret;

    *attr = 0;
    ret = lstat( path, st );
    if (ret == -1) return ret;
    if (S_ISLNK( st->st_mode ))
    {
        ret = stat( path, st );
        if (ret == -1) return ret;
        /* is a symbolic link and a directory, consider these "reparse points" */
        if (S_ISDIR( st->st_mode )) *attr |= FILE_ATTRIBUTE_REPARSE_POINT;
    }
    else if (S_ISDIR( st->st_mode ) && (parent_path = malloc( strlen(path) + 4 )))
    {
        struct stat parent_st;

        /* consider mount points to be reparse points (IO_REPARSE_TAG_MOUNT_POINT) */
        strcpy( parent_path, path );
        strcat( parent_path, "/.." );
        if (!stat( parent_path, &parent_st )
                && (st->st_dev != parent_st.st_dev || st->st_ino == parent_st.st_ino))
            *attr |= FILE_ATTRIBUTE_REPARSE_POINT;

        free( parent_path );
    }
    *attr |= get_file_attributes( st );
    return ret;
}


#if defined(__ANDROID__) && !defined(HAVE_FUTIMENS)
static int futimens( int fd, const struct timespec spec[2] )
{
    return syscall( __NR_utimensat, fd, NULL, spec, 0 );
}
#define HAVE_FUTIMENS
#endif  /* __ANDROID__ */

#ifndef UTIME_OMIT
#define UTIME_OMIT ((1 << 30) - 2)
#endif

static BOOL set_file_times_precise( int fd, const LARGE_INTEGER *mtime,
                                    const LARGE_INTEGER *atime, NTSTATUS *status )
{
#ifdef HAVE_FUTIMENS
    struct timespec tv[2];

    tv[0].tv_sec = tv[1].tv_sec = 0;
    tv[0].tv_nsec = tv[1].tv_nsec = UTIME_OMIT;
    if (atime->QuadPart)
    {
        tv[0].tv_sec = atime->QuadPart / 10000000 - SECS_1601_TO_1970;
        tv[0].tv_nsec = (atime->QuadPart % 10000000) * 100;
    }
    if (mtime->QuadPart)
    {
        tv[1].tv_sec = mtime->QuadPart / 10000000 - SECS_1601_TO_1970;
        tv[1].tv_nsec = (mtime->QuadPart % 10000000) * 100;
    }
#ifdef __APPLE__
    if (!&futimens) return FALSE;
#endif
    if (futimens( fd, tv ) == -1) *status = errno_to_status( errno );
    else *status = STATUS_SUCCESS;
    return TRUE;
#else
    return FALSE;
#endif
}


static NTSTATUS set_file_times( int fd, const LARGE_INTEGER *mtime, const LARGE_INTEGER *atime )
{
    NTSTATUS status = STATUS_SUCCESS;
#if defined(HAVE_FUTIMES) || defined(HAVE_FUTIMESAT)
    struct timeval tv[2];
    struct stat st;
#endif

    if (set_file_times_precise( fd, mtime, atime, &status ))
        return status;

#if defined(HAVE_FUTIMES) || defined(HAVE_FUTIMESAT)
    if (!atime->QuadPart || !mtime->QuadPart)
    {

        tv[0].tv_sec = tv[0].tv_usec = 0;
        tv[1].tv_sec = tv[1].tv_usec = 0;
        if (!fstat( fd, &st ))
        {
            tv[0].tv_sec = st.st_atime;
            tv[1].tv_sec = st.st_mtime;
#ifdef HAVE_STRUCT_STAT_ST_ATIM
            tv[0].tv_usec = st.st_atim.tv_nsec / 1000;
#elif defined(HAVE_STRUCT_STAT_ST_ATIMESPEC)
            tv[0].tv_usec = st.st_atimespec.tv_nsec / 1000;
#endif
#ifdef HAVE_STRUCT_STAT_ST_MTIM
            tv[1].tv_usec = st.st_mtim.tv_nsec / 1000;
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC)
            tv[1].tv_usec = st.st_mtimespec.tv_nsec / 1000;
#endif
        }
    }
    if (atime->QuadPart)
    {
        tv[0].tv_sec = atime->QuadPart / 10000000 - SECS_1601_TO_1970;
        tv[0].tv_usec = (atime->QuadPart % 10000000) / 10;
    }
    if (mtime->QuadPart)
    {
        tv[1].tv_sec = mtime->QuadPart / 10000000 - SECS_1601_TO_1970;
        tv[1].tv_usec = (mtime->QuadPart % 10000000) / 10;
    }
#ifdef HAVE_FUTIMES
    if (futimes( fd, tv ) == -1) status = errno_to_status( errno );
#elif defined(HAVE_FUTIMESAT)
    if (futimesat( fd, NULL, tv ) == -1) status = errno_to_status( errno );
#endif

#else  /* HAVE_FUTIMES || HAVE_FUTIMESAT */
    FIXME( "setting file times not supported\n" );
    status = STATUS_NOT_IMPLEMENTED;
#endif
    return status;
}


static inline void get_file_times( const struct stat *st, LARGE_INTEGER *mtime, LARGE_INTEGER *ctime,
                                   LARGE_INTEGER *atime, LARGE_INTEGER *creation )
{
    mtime->QuadPart = st->st_mtime * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
    ctime->QuadPart = st->st_ctime * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
    atime->QuadPart = st->st_atime * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
#ifdef HAVE_STRUCT_STAT_ST_MTIM
    mtime->QuadPart += st->st_mtim.tv_nsec / 100;
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC)
    mtime->QuadPart += st->st_mtimespec.tv_nsec / 100;
#endif
#ifdef HAVE_STRUCT_STAT_ST_CTIM
    ctime->QuadPart += st->st_ctim.tv_nsec / 100;
#elif defined(HAVE_STRUCT_STAT_ST_CTIMESPEC)
    ctime->QuadPart += st->st_ctimespec.tv_nsec / 100;
#endif
#ifdef HAVE_STRUCT_STAT_ST_ATIM
    atime->QuadPart += st->st_atim.tv_nsec / 100;
#elif defined(HAVE_STRUCT_STAT_ST_ATIMESPEC)
    atime->QuadPart += st->st_atimespec.tv_nsec / 100;
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
    creation->QuadPart = st->st_birthtime * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIM
    creation->QuadPart += st->st_birthtim.tv_nsec / 100;
#elif defined(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC)
    creation->QuadPart += st->st_birthtimespec.tv_nsec / 100;
#endif
#elif defined(HAVE_STRUCT_STAT___ST_BIRTHTIME)
    creation->QuadPart = st->__st_birthtime * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
#ifdef HAVE_STRUCT_STAT___ST_BIRTHTIM
    creation->QuadPart += st->__st_birthtim.tv_nsec / 100;
#endif
#else
    *creation = *mtime;
#endif
}


/* fill in the file information that depends on the stat and attribute info */
static NTSTATUS fill_file_info( const struct stat *st, ULONG attr, void *ptr,
                                FILE_INFORMATION_CLASS class )
{
    switch (class)
    {
    case FileBasicInformation:
        {
            FILE_BASIC_INFORMATION *info = ptr;

            get_file_times( st, &info->LastWriteTime, &info->ChangeTime,
                            &info->LastAccessTime, &info->CreationTime );
            info->FileAttributes = attr;
        }
        break;
    case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION *info = ptr;

            if ((info->Directory = S_ISDIR(st->st_mode)))
            {
                info->AllocationSize.QuadPart = 0;
                info->EndOfFile.QuadPart      = 0;
                info->NumberOfLinks           = 1;
            }
            else
            {
                info->AllocationSize.QuadPart = (ULONGLONG)st->st_blocks * 512;
                info->EndOfFile.QuadPart      = st->st_size;
                info->NumberOfLinks           = st->st_nlink;
            }
        }
        break;
    case FileInternalInformation:
        {
            FILE_INTERNAL_INFORMATION *info = ptr;
            info->IndexNumber.QuadPart = st->st_ino;
        }
        break;
    case FileEndOfFileInformation:
        {
            FILE_END_OF_FILE_INFORMATION *info = ptr;
            info->EndOfFile.QuadPart = S_ISDIR(st->st_mode) ? 0 : st->st_size;
        }
        break;
    case FileAllInformation:
        {
            FILE_ALL_INFORMATION *info = ptr;
            fill_file_info( st, attr, &info->BasicInformation, FileBasicInformation );
            fill_file_info( st, attr, &info->StandardInformation, FileStandardInformation );
            fill_file_info( st, attr, &info->InternalInformation, FileInternalInformation );
        }
        break;
    /* all directory structures start with the FileDirectoryInformation layout */
    case FileBothDirectoryInformation:
    case FileFullDirectoryInformation:
    case FileDirectoryInformation:
        {
            FILE_DIRECTORY_INFORMATION *info = ptr;

            get_file_times( st, &info->LastWriteTime, &info->ChangeTime,
                            &info->LastAccessTime, &info->CreationTime );
            if (S_ISDIR(st->st_mode))
            {
                info->AllocationSize.QuadPart = 0;
                info->EndOfFile.QuadPart      = 0;
            }
            else
            {
                info->AllocationSize.QuadPart = (ULONGLONG)st->st_blocks * 512;
                info->EndOfFile.QuadPart      = st->st_size;
            }
            info->FileAttributes = attr;
        }
        break;
    case FileIdFullDirectoryInformation:
        {
            FILE_ID_FULL_DIRECTORY_INFORMATION *info = ptr;
            info->FileId.QuadPart = st->st_ino;
            fill_file_info( st, attr, info, FileDirectoryInformation );
        }
        break;
    case FileIdBothDirectoryInformation:
        {
            FILE_ID_BOTH_DIRECTORY_INFORMATION *info = ptr;
            info->FileId.QuadPart = st->st_ino;
            fill_file_info( st, attr, info, FileDirectoryInformation );
        }
        break;
    case FileIdGlobalTxDirectoryInformation:
        {
            FILE_ID_GLOBAL_TX_DIR_INFORMATION *info = ptr;
            info->FileId.QuadPart = st->st_ino;
            fill_file_info( st, attr, info, FileDirectoryInformation );
        }
        break;

    default:
        return STATUS_INVALID_INFO_CLASS;
    }
    return STATUS_SUCCESS;
}


static NTSTATUS server_get_unix_name( HANDLE handle, char **unix_name )
{
    data_size_t size = 1024;
    NTSTATUS ret;
    char *name;

    for (;;)
    {
        if (!(name = malloc( size + 1 ))) return STATUS_NO_MEMORY;

        SERVER_START_REQ( get_handle_unix_name )
        {
            req->handle = wine_server_obj_handle( handle );
            wine_server_set_reply( req, name, size );
            ret = wine_server_call( req );
            size = reply->name_len;
        }
        SERVER_END_REQ;

        if (!ret)
        {
            name[size] = 0;
            *unix_name = name;
            break;
        }
        free( name );
        if (ret != STATUS_BUFFER_OVERFLOW) break;
    }
    return ret;
}

static NTSTATUS fill_name_info( const char *unix_name, FILE_NAME_INFORMATION *info, LONG *name_len )
{
    WCHAR *nt_name;
    NTSTATUS status;

    if (!(status = unix_to_nt_file_name( unix_name, &nt_name )))
    {
        const WCHAR *ptr = nt_name;
        const WCHAR *end = ptr + wcslen( nt_name );

        /* Skip the volume mount point. */
        while (ptr != end && *ptr == '\\') ++ptr;
        while (ptr != end && *ptr != '\\') ++ptr;
        while (ptr != end && *ptr == '\\') ++ptr;
        while (ptr != end && *ptr != '\\') ++ptr;

        info->FileNameLength = (end - ptr) * sizeof(WCHAR);
        if (*name_len < info->FileNameLength) status = STATUS_BUFFER_OVERFLOW;
        else *name_len = info->FileNameLength;

        memcpy( info->FileName, ptr, *name_len );
        free( nt_name );
    }

    return status;
}


static NTSTATUS server_get_file_info( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer,
                                      ULONG length, FILE_INFORMATION_CLASS info_class )
{
    SERVER_START_REQ( get_file_info )
    {
        req->handle = wine_server_obj_handle( handle );
        req->info_class = info_class;
        wine_server_set_reply( req, buffer, length );
        io->u.Status = wine_server_call( req );
        io->Information = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;
    if (io->u.Status == STATUS_NOT_IMPLEMENTED)
        FIXME( "Unsupported info class %x\n", info_class );
    return io->u.Status;

}


/* retrieve device/inode number for all the drives */
static unsigned int get_drives_info( struct file_identity info[MAX_DOS_DRIVES] )
{
    static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
    static struct file_identity cache[MAX_DOS_DRIVES];
    static time_t last_update;
    static unsigned int nb_drives;
    unsigned int ret;
    time_t now = time(NULL);

    mutex_lock( &cache_mutex );
    if (now != last_update)
    {
        char *buffer, *p;
        struct stat st;
        unsigned int i;

        if ((buffer = malloc( strlen(config_dir) + sizeof("/dosdevices/a:") )))
        {
            strcpy( buffer, config_dir );
            strcat( buffer, "/dosdevices/a:" );
            p = buffer + strlen(buffer) - 2;

            for (i = nb_drives = 0; i < MAX_DOS_DRIVES; i++)
            {
                *p = 'a' + i;
                if (!stat( buffer, &st ))
                {
                    cache[i].dev = st.st_dev;
                    cache[i].ino = st.st_ino;
                    nb_drives++;
                }
                else
                {
                    cache[i].dev = 0;
                    cache[i].ino = 0;
                }
            }
            free( buffer );
        }
        last_update = now;
    }
    memcpy( info, cache, sizeof(cache) );
    ret = nb_drives;
    mutex_unlock( &cache_mutex );
    return ret;
}


/* Find a DOS device which can act as the root of "path".
 * Similar to find_drive_root(), but returns -1 instead of crossing volumes. */
static int find_dos_device( const char *path )
{
    int len = strlen(path);
    int drive;
    char *buffer;
    struct stat st;
    struct file_identity info[MAX_DOS_DRIVES];
    dev_t dev_id;

    if (!get_drives_info( info )) return -1;

    if (stat( path, &st ) < 0) return -1;
    dev_id = st.st_dev;

    /* strip off trailing slashes */
    while (len > 1 && path[len - 1] == '/') len--;

    /* make a copy of the path */
    if (!(buffer = malloc( len + 1 ))) return -1;
    memcpy( buffer, path, len );
    buffer[len] = 0;

    for (;;)
    {
        if (!stat( buffer, &st ) && S_ISDIR( st.st_mode ))
        {
            if (st.st_dev != dev_id) break;

            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
                if ((info[drive].dev == st.st_dev) && (info[drive].ino == st.st_ino))
                {
                    TRACE( "%s -> drive %c:, root=%s, name=%s\n",
                           debugstr_a(path), 'A' + drive, debugstr_a(buffer), debugstr_a(path + len));
                    free( buffer );
                    return drive;
                }
            }
        }
        if (len <= 1) break;  /* reached root */
        while (len > 1 && path[len - 1] != '/') len--;
        while (len > 1 && path[len - 1] == '/') len--;
        buffer[len] = 0;
    }
    free( buffer );
    return -1;
}

static NTSTATUS get_mountmgr_fs_info( HANDLE handle, int fd, struct mountmgr_unix_drive *drive, ULONG size )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    char *unix_name;
    IO_STATUS_BLOCK io;
    HANDLE mountmgr;
    NTSTATUS status;
    int letter;

    if ((status = server_get_unix_name( handle, &unix_name ))) return status;
    letter = find_dos_device( unix_name );
    free( unix_name );

    if (letter == -1)
    {
        struct stat st;

        fstat( fd, &st );
        drive->unix_dev = st.st_dev;
        drive->letter = 0;
    }
    else
        drive->letter = 'a' + letter;

    string.Buffer = (WCHAR *)MOUNTMGR_DEVICE_NAME;
    string.Length = sizeof(MOUNTMGR_DEVICE_NAME) - sizeof(WCHAR);
    InitializeObjectAttributes( &attr, &string, 0, NULL, NULL );
    status = NtOpenFile( &mountmgr, GENERIC_READ | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT );
    if (status) return status;

    status = NtDeviceIoControlFile( mountmgr, NULL, NULL, NULL, &io, IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE,
                                    drive, sizeof(*drive), drive, size );
    NtClose( mountmgr );
    if (status == STATUS_BUFFER_OVERFLOW) status = STATUS_SUCCESS;
    else if (status) WARN("failed to retrieve filesystem type from mountmgr, status %#x\n", status);
    return status;
}


/***********************************************************************
 *           get_dir_data_entry
 *
 * Return a directory entry from the cached data.
 */
static NTSTATUS get_dir_data_entry( struct dir_data *dir_data, void *info_ptr, IO_STATUS_BLOCK *io,
                                    ULONG max_length, FILE_INFORMATION_CLASS class,
                                    union file_directory_info **last_info )
{
    const struct dir_data_names *names = &dir_data->names[dir_data->pos];
    union file_directory_info *info;
    struct stat st;
    ULONG name_len, start, dir_size, attributes;

    if (get_file_info( names->unix_name, &st, &attributes ) == -1)
    {
        TRACE( "file no longer exists %s\n", names->unix_name );
        return STATUS_SUCCESS;
    }
    if (is_ignored_file( &st ))
    {
        TRACE( "ignoring file %s\n", names->unix_name );
        return STATUS_SUCCESS;
    }
    start = dir_info_align( io->Information );
    dir_size = dir_info_size( class, 0 );
    if (start + dir_size > max_length) return STATUS_MORE_ENTRIES;

    max_length -= start + dir_size;
    name_len = wcslen( names->long_name ) * sizeof(WCHAR);
    /* if this is not the first entry, fail; the first entry is always returned (but truncated) */
    if (*last_info && name_len > max_length) return STATUS_MORE_ENTRIES;

    info = (union file_directory_info *)((char *)info_ptr + start);
    info->dir.NextEntryOffset = 0;
    info->dir.FileIndex = 0;  /* NTFS always has 0 here, so let's not bother with it */

    /* all the structures except FileNamesInformation start with a FileDirectoryInformation layout */
    if (class != FileNamesInformation)
    {
        if (st.st_dev != dir_data->id.dev) st.st_ino = 0;  /* ignore inode if on a different device */

        if (!show_dot_files && names->long_name[0] == '.' && names->long_name[1] &&
            (names->long_name[1] != '.' || names->long_name[2]))
            attributes |= FILE_ATTRIBUTE_HIDDEN;

        fill_file_info( &st, attributes, info, class );
    }

    switch (class)
    {
    case FileDirectoryInformation:
        info->dir.FileNameLength = name_len;
        break;

    case FileFullDirectoryInformation:
        info->full.EaSize = 0; /* FIXME */
        info->full.FileNameLength = name_len;
        break;

    case FileIdFullDirectoryInformation:
        info->id_full.EaSize = 0; /* FIXME */
        info->id_full.FileNameLength = name_len;
        break;

    case FileBothDirectoryInformation:
        info->both.EaSize = 0; /* FIXME */
        info->both.ShortNameLength = wcslen( names->short_name ) * sizeof(WCHAR);
        memcpy( info->both.ShortName, names->short_name, info->both.ShortNameLength );
        info->both.FileNameLength = name_len;
        break;

    case FileIdBothDirectoryInformation:
        info->id_both.EaSize = 0; /* FIXME */
        info->id_both.ShortNameLength = wcslen( names->short_name ) * sizeof(WCHAR);
        memcpy( info->id_both.ShortName, names->short_name, info->id_both.ShortNameLength );
        info->id_both.FileNameLength = name_len;
        break;

    case FileIdGlobalTxDirectoryInformation:
        info->id_tx.TxInfoFlags = 0;
        info->id_tx.FileNameLength = name_len;
        break;

    case FileNamesInformation:
        info->names.FileNameLength = name_len;
        break;

    default:
        assert(0);
        return 0;
    }

    memcpy( (char *)info + dir_size, names->long_name, min( name_len, max_length ) );
    io->Information = start + dir_size + min( name_len, max_length );
    if (*last_info) (*last_info)->next = (char *)info - (char *)*last_info;
    *last_info = info;
    return name_len > max_length ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
}

#ifdef VFAT_IOCTL_READDIR_BOTH

/***********************************************************************
 *           read_directory_vfat
 *
 * Read a directory using the VFAT ioctl; helper for NtQueryDirectoryFile.
 */
static NTSTATUS read_directory_data_vfat( struct dir_data *data, int fd, const UNICODE_STRING *mask )
{
    char *short_name, *long_name;
    KERNEL_DIRENT de[2];
    NTSTATUS status = STATUS_NO_MEMORY;
    off_t old_pos = lseek( fd, 0, SEEK_CUR );

    lseek( fd, 0, SEEK_SET );

    if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) == -1)
    {
        if (errno != ENOENT)
        {
            status = STATUS_NOT_SUPPORTED;
            goto done;
        }
        de[0].d_reclen = 0;
    }

    if (!append_entry( data, ".", NULL, mask )) goto done;
    if (!append_entry( data, "..", NULL, mask )) goto done;

    while (de[0].d_reclen)
    {
        if (strcmp( de[0].d_name, "." ) && strcmp( de[0].d_name, ".." ))
        {
            if (de[1].d_name[0])
            {
                short_name = de[0].d_name;
                long_name = de[1].d_name;
            }
            else
            {
                long_name = de[0].d_name;
                short_name = NULL;
            }
            if (!append_entry( data, long_name, short_name, mask )) goto done;
        }
        if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) == -1) break;
    }
    status = STATUS_SUCCESS;
done:
    lseek( fd, old_pos, SEEK_SET );
    return status;
}
#endif /* VFAT_IOCTL_READDIR_BOTH */


#ifdef HAVE_GETATTRLIST
/***********************************************************************
 *           read_directory_getattrlist
 *
 * Read a single file from a directory by determining whether the file
 * identified by mask exists using getattrlist.
 */
static NTSTATUS read_directory_data_getattrlist( struct dir_data *data, const char *unix_name )
{
    struct attrlist attrlist;
#include "pshpack4.h"
    struct
    {
        u_int32_t length;
        struct attrreference name_reference;
        fsobj_type_t type;
        char name[NAME_MAX * 3 + 1];
    } buffer;
#include "poppack.h"

    memset( &attrlist, 0, sizeof(attrlist) );
    attrlist.bitmapcount = ATTR_BIT_MAP_COUNT;
    attrlist.commonattr = ATTR_CMN_NAME | ATTR_CMN_OBJTYPE;
    if (getattrlist( unix_name, &attrlist, &buffer, sizeof(buffer), FSOPT_NOFOLLOW ) == -1)
        return STATUS_NO_SUCH_FILE;
    /* If unix_name named a symlink, the above may have succeeded even if the symlink is broken.
       Check that with another call without FSOPT_NOFOLLOW.  We don't ask for any attributes. */
    if (buffer.type == VLNK)
    {
        u_int32_t dummy;
        attrlist.commonattr = 0;
        if (getattrlist( unix_name, &attrlist, &dummy, sizeof(dummy), 0 ) == -1)
            return STATUS_NO_SUCH_FILE;
    }

    TRACE( "found %s\n", buffer.name );

    if (!append_entry( data, buffer.name, NULL, NULL )) return STATUS_NO_MEMORY;

    return STATUS_SUCCESS;
}
#endif  /* HAVE_GETATTRLIST */


/***********************************************************************
 *           read_directory_stat
 *
 * Read a single file from a directory by determining whether the file
 * identified by mask exists using stat.
 */
static NTSTATUS read_directory_data_stat( struct dir_data *data, const char *unix_name )
{
    struct stat st;

    /* if the file system is not case sensitive we can't find the actual name through stat() */
    if (!get_dir_case_sensitivity(".")) return STATUS_NO_SUCH_FILE;
    if (stat( unix_name, &st ) == -1) return STATUS_NO_SUCH_FILE;

    TRACE( "found %s\n", unix_name );

    if (!append_entry( data, unix_name, NULL, NULL )) return STATUS_NO_MEMORY;

    return STATUS_SUCCESS;
}


/***********************************************************************
 *           read_directory_readdir
 *
 * Read a directory using the POSIX readdir interface; helper for NtQueryDirectoryFile.
 */
static NTSTATUS read_directory_data_readdir( struct dir_data *data, const UNICODE_STRING *mask )
{
    struct dirent *de;
    NTSTATUS status = STATUS_NO_MEMORY;
    DIR *dir = opendir( "." );

    if (!dir) return STATUS_NO_SUCH_FILE;

    if (!append_entry( data, ".", NULL, mask )) goto done;
    if (!append_entry( data, "..", NULL, mask )) goto done;
    while ((de = readdir( dir )))
    {
        if (!strcmp( de->d_name, "." ) || !strcmp( de->d_name, ".." )) continue;
        if (!append_entry( data, de->d_name, NULL, mask )) goto done;
    }
    status = STATUS_SUCCESS;

done:
    closedir( dir );
    return status;
}


/***********************************************************************
 *           read_directory_data
 *
 * Read the full contents of a directory, using one of the above helper functions.
 */
static NTSTATUS read_directory_data( struct dir_data *data, int fd, const UNICODE_STRING *mask )
{
    NTSTATUS status;

#ifdef VFAT_IOCTL_READDIR_BOTH
    if (!(status = read_directory_data_vfat( data, fd, mask ))) return status;
#endif

    if (!has_wildcard( mask ))
    {
        /* convert the mask to a Unix name and check for it */
        char unix_name[MAX_DIR_ENTRY_LEN * 3 + 1];
        int ret = ntdll_wcstoumbs( mask->Buffer, mask->Length / sizeof(WCHAR),
                                   unix_name, sizeof(unix_name) - 1, TRUE );
        if (ret > 0)
        {
            unix_name[ret] = 0;
#ifdef HAVE_GETATTRLIST
            if (!(status = read_directory_data_getattrlist( data, unix_name ))) return status;
#endif
            if (!(status = read_directory_data_stat( data, unix_name ))) return status;
        }
    }

    return read_directory_data_readdir( data, mask );
}


/* compare file names for directory sorting */
static int name_compare( const void *a, const void *b )
{
    const struct dir_data_names *file_a = (const struct dir_data_names *)a;
    const struct dir_data_names *file_b = (const struct dir_data_names *)b;
    int ret = wcsicmp( file_a->long_name, file_b->long_name );
    if (!ret) ret = wcscmp( file_a->long_name, file_b->long_name );
    return ret;
}


/***********************************************************************
 *           init_cached_dir_data
 *
 * Initialize the cached directory contents.
 */
static NTSTATUS init_cached_dir_data( struct dir_data **data_ret, int fd, const UNICODE_STRING *mask )
{
    struct dir_data *data;
    struct stat st;
    NTSTATUS status;
    unsigned int i;

    if (!(data = calloc( 1, sizeof(*data) ))) return STATUS_NO_MEMORY;

    if ((status = read_directory_data( data, fd, mask )))
    {
        free_dir_data( data );
        return status;
    }

    /* sort filenames, but not "." and ".." */
    i = 0;
    if (i < data->count && !strcmp( data->names[i].unix_name, "." )) i++;
    if (i < data->count && !strcmp( data->names[i].unix_name, ".." )) i++;
    if (i < data->count) qsort( data->names + i, data->count - i, sizeof(*data->names), name_compare );

    if (data->count)
    {
        fstat( fd, &st );
        data->id.dev = st.st_dev;
        data->id.ino = st.st_ino;
    }

    TRACE( "mask %s found %u files\n", debugstr_us( mask ), data->count );
    for (i = 0; i < data->count; i++)
        TRACE( "%s %s\n", debugstr_w(data->names[i].long_name), debugstr_w(data->names[i].short_name) );

    *data_ret = data;
    return data->count ? STATUS_SUCCESS : STATUS_NO_SUCH_FILE;
}


/***********************************************************************
 *           get_cached_dir_data
 *
 * Retrieve the cached directory data, or initialize it if necessary.
 */
static NTSTATUS get_cached_dir_data( HANDLE handle, struct dir_data **data_ret, int fd,
                                     const UNICODE_STRING *mask )
{
    unsigned int i;
    int entry = -1, free_entries[16];
    NTSTATUS status;

    SERVER_START_REQ( get_directory_cache_entry )
    {
        req->handle = wine_server_obj_handle( handle );
        wine_server_set_reply( req, free_entries, sizeof(free_entries) );
        if (!(status = wine_server_call( req ))) entry = reply->entry;

        for (i = 0; i < wine_server_reply_size( reply ) / sizeof(*free_entries); i++)
        {
            int free_idx = free_entries[i];
            if (free_idx < dir_data_cache_size)
            {
                free_dir_data( dir_data_cache[free_idx] );
                dir_data_cache[free_idx] = NULL;
            }
        }
    }
    SERVER_END_REQ;

    if (status)
    {
        if (status == STATUS_SHARING_VIOLATION) FIXME( "shared directory handle not supported yet\n" );
        return status;
    }

    if (entry >= dir_data_cache_size)
    {
        unsigned int size = max( dir_data_cache_initial_size, max( dir_data_cache_size * 2, entry + 1 ) );
        struct dir_data **new_cache = realloc( dir_data_cache, size * sizeof(*new_cache) );

        if (!new_cache) return STATUS_NO_MEMORY;
        memset( new_cache + dir_data_cache_size, 0, (size - dir_data_cache_size) * sizeof(*new_cache) );
        dir_data_cache = new_cache;
        dir_data_cache_size = size;
    }

    if (!dir_data_cache[entry]) status = init_cached_dir_data( &dir_data_cache[entry], fd, mask );

    *data_ret = dir_data_cache[entry];
    return status;
}


/******************************************************************************
 *              NtQueryDirectoryFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDirectoryFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc_routine,
                                      void *apc_context, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                      FILE_INFORMATION_CLASS info_class, BOOLEAN single_entry,
                                      UNICODE_STRING *mask, BOOLEAN restart_scan )
{
    int cwd, fd, needs_close;
    enum server_fd_type type;
    struct dir_data *data;
    NTSTATUS status;

    TRACE("(%p %p %p %p %p %p 0x%08x 0x%08x 0x%08x %s 0x%08x\n",
          handle, event, apc_routine, apc_context, io, buffer,
          length, info_class, single_entry, debugstr_us(mask),
          restart_scan);

    if (event || apc_routine)
    {
        FIXME( "Unsupported yet option\n" );
        return STATUS_NOT_IMPLEMENTED;
    }
    switch (info_class)
    {
    case FileDirectoryInformation:
    case FileBothDirectoryInformation:
    case FileFullDirectoryInformation:
    case FileIdBothDirectoryInformation:
    case FileIdFullDirectoryInformation:
    case FileIdGlobalTxDirectoryInformation:
    case FileNamesInformation:
        if (length < dir_info_align( dir_info_size( info_class, 1 ))) return STATUS_INFO_LENGTH_MISMATCH;
        break;
    case FileObjectIdInformation:
        if (length != sizeof(FILE_OBJECTID_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;
        return STATUS_INVALID_INFO_CLASS;
    case FileQuotaInformation:
        if (length != sizeof(FILE_QUOTA_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;
        return STATUS_INVALID_INFO_CLASS;
    case FileReparsePointInformation:
        if (length != sizeof(FILE_REPARSE_POINT_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;
        return STATUS_INVALID_INFO_CLASS;
    default:
        return STATUS_INVALID_INFO_CLASS;
    }
    if (!buffer) return STATUS_ACCESS_VIOLATION;

    if ((status = server_get_unix_fd( handle, FILE_LIST_DIRECTORY, &fd, &needs_close, &type, NULL )))
        return status;

    if (type != FD_TYPE_DIR)
    {
        if (needs_close) close( fd );
        return STATUS_INVALID_PARAMETER;
    }

    io->Information = 0;

    mutex_lock( &dir_mutex );

    cwd = open( ".", O_RDONLY );
    if (fchdir( fd ) != -1)
    {
        if (!(status = get_cached_dir_data( handle, &data, fd, mask )))
        {
            union file_directory_info *last_info = NULL;

            if (restart_scan) data->pos = 0;

            while (!status && data->pos < data->count)
            {
                status = get_dir_data_entry( data, buffer, io, length, info_class, &last_info );
                if (!status || status == STATUS_BUFFER_OVERFLOW) data->pos++;
                if (single_entry && last_info) break;
            }

            if (!last_info) status = STATUS_NO_MORE_FILES;
            else if (status == STATUS_MORE_ENTRIES) status = STATUS_SUCCESS;

            io->u.Status = status;
        }
        if (cwd == -1 || fchdir( cwd ) == -1) chdir( "/" );
    }
    else status = errno_to_status( errno );

    mutex_unlock( &dir_mutex );

    if (needs_close) close( fd );
    if (cwd != -1) close( cwd );
    TRACE( "=> %x (%ld)\n", status, io->Information );
    return status;
}


/***********************************************************************
 *           find_file_in_dir
 *
 * Find a file in a directory the hard way, by doing a case-insensitive search.
 * The file found is appended to unix_name at pos.
 * There must be at least MAX_DIR_ENTRY_LEN+2 chars available at pos.
 */
static NTSTATUS find_file_in_dir( char *unix_name, int pos, const WCHAR *name, int length,
                                  BOOLEAN check_case, BOOLEAN *is_win_dir )
{
    WCHAR buffer[MAX_DIR_ENTRY_LEN];
    BOOLEAN is_name_8_dot_3;
    DIR *dir;
    struct dirent *de;
    struct stat st;
    int ret;

    /* try a shortcut for this directory */

    unix_name[pos++] = '/';
    ret = ntdll_wcstoumbs( name, length, unix_name + pos, MAX_DIR_ENTRY_LEN + 1, TRUE );
    if (ret >= 0 && ret <= MAX_DIR_ENTRY_LEN)
    {
        unix_name[pos + ret] = 0;
        if (!stat( unix_name, &st ))
        {
            if (is_win_dir) *is_win_dir = is_same_file( &windir, &st );
            return STATUS_SUCCESS;
        }
    }
    if (check_case) goto not_found;  /* we want an exact match */

    if (pos > 1) unix_name[pos - 1] = 0;
    else unix_name[1] = 0;  /* keep the initial slash */

    /* check if it fits in 8.3 so that we don't look for short names if we won't need them */

    is_name_8_dot_3 = is_legal_8dot3_name( name, length );
#ifndef VFAT_IOCTL_READDIR_BOTH
    is_name_8_dot_3 = is_name_8_dot_3 && length >= 8 && name[4] == '~';
#endif

    if (!is_name_8_dot_3 && !get_dir_case_sensitivity( unix_name )) goto not_found;

    /* now look for it through the directory */

#ifdef VFAT_IOCTL_READDIR_BOTH
    if (is_name_8_dot_3)
    {
        int fd = open( unix_name, O_RDONLY | O_DIRECTORY );
        if (fd != -1)
        {
            KERNEL_DIRENT kde[2];

            if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)kde ) != -1)
            {
                unix_name[pos - 1] = '/';
                while (kde[0].d_reclen)
                {
                    if (kde[1].d_name[0])
                    {
                        ret = ntdll_umbstowcs( kde[1].d_name, strlen(kde[1].d_name),
                                               buffer, MAX_DIR_ENTRY_LEN );
                        if (ret == length && !wcsnicmp( buffer, name, ret ))
                        {
                            strcpy( unix_name + pos, kde[1].d_name );
                            close( fd );
                            goto success;
                        }
                    }
                    ret = ntdll_umbstowcs( kde[0].d_name, strlen(kde[0].d_name),
                                           buffer, MAX_DIR_ENTRY_LEN );
                    if (ret == length && !wcsnicmp( buffer, name, ret ))
                    {
                        strcpy( unix_name + pos,
                                kde[1].d_name[0] ? kde[1].d_name : kde[0].d_name );
                        close( fd );
                        goto success;
                    }
                    if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)kde ) == -1)
                    {
                        close( fd );
                        goto not_found;
                    }
                }
                /* if that did not work, restore previous state of unix_name */
                unix_name[pos - 1] = 0;
            }
            close( fd );
        }
        /* fall through to normal handling */
    }
#endif /* VFAT_IOCTL_READDIR_BOTH */

    if (!(dir = opendir( unix_name ))) return errno_to_status( errno );

    unix_name[pos - 1] = '/';
    while ((de = readdir( dir )))
    {
        ret = ntdll_umbstowcs( de->d_name, strlen(de->d_name), buffer, MAX_DIR_ENTRY_LEN );
        if (ret == length && !wcsnicmp( buffer, name, ret ))
        {
            strcpy( unix_name + pos, de->d_name );
            closedir( dir );
            goto success;
        }

        if (!is_name_8_dot_3) continue;

        if (!is_legal_8dot3_name( buffer, ret ))
        {
            WCHAR short_nameW[12];
            ret = hash_short_file_name( buffer, ret, short_nameW );
            if (ret == length && !wcsnicmp( short_nameW, name, length ))
            {
                strcpy( unix_name + pos, de->d_name );
                closedir( dir );
                goto success;
            }
        }
    }
    closedir( dir );

not_found:
    unix_name[pos - 1] = 0;
    return STATUS_OBJECT_PATH_NOT_FOUND;

success:
    if (is_win_dir && !stat( unix_name, &st )) *is_win_dir = is_same_file( &windir, &st );
    return STATUS_SUCCESS;
}


#ifndef _WIN64

static const WCHAR catrootW[] = {'s','y','s','t','e','m','3','2','\\','c','a','t','r','o','o','t',0};
static const WCHAR catroot2W[] = {'s','y','s','t','e','m','3','2','\\','c','a','t','r','o','o','t','2',0};
static const WCHAR driversstoreW[] = {'s','y','s','t','e','m','3','2','\\','d','r','i','v','e','r','s','s','t','o','r','e',0};
static const WCHAR driversetcW[] = {'s','y','s','t','e','m','3','2','\\','d','r','i','v','e','r','s','\\','e','t','c',0};
static const WCHAR logfilesW[] = {'s','y','s','t','e','m','3','2','\\','l','o','g','f','i','l','e','s',0};
static const WCHAR spoolW[] = {'s','y','s','t','e','m','3','2','\\','s','p','o','o','l',0};
static const WCHAR system32W[] = {'s','y','s','t','e','m','3','2',0};
static const WCHAR sysnativeW[] = {'s','y','s','n','a','t','i','v','e',0};
static const WCHAR regeditW[] = {'r','e','g','e','d','i','t','.','e','x','e',0};

static struct
{
    const WCHAR *source;
    const char *unix_target;
} redirects[] =
{
    { catrootW, NULL },
    { catroot2W, NULL },
    { driversstoreW, NULL },
    { driversetcW, NULL },
    { logfilesW, NULL },
    { spoolW, NULL },
    { system32W, "syswow64" },
    { sysnativeW, "system32" },
    { regeditW, "syswow64/regedit.exe" }
};

static unsigned int nb_redirects;


/***********************************************************************
 *           init_redirects
 */
static void init_redirects(void)
{
    static const char windows_dir[] = "/dosdevices/c:/windows";
    char *dir;
    struct stat st;

    if (!(dir = malloc( strlen(config_dir) + sizeof(windows_dir) ))) return;
    strcpy( dir, config_dir );
    strcat( dir, windows_dir );
    if (!stat( dir, &st ))
    {
        windir.dev = st.st_dev;
        windir.ino = st.st_ino;
        nb_redirects = ARRAY_SIZE( redirects );
    }
    else ERR( "%s: %s\n", dir, strerror(errno) );
    free( dir );

}


/***********************************************************************
 *           match_redirect
 *
 * Check if path matches a redirect name. If yes, return matched length.
 */
static int match_redirect( const WCHAR *path, int len, const WCHAR *redir, BOOLEAN check_case )
{
    int i = 0;

    while (i < len)
    {
        int start = i;
        while (i < len && !IS_SEPARATOR(path[i])) i++;
        if (check_case)
        {
            if (wcsncmp( path + start, redir, i - start )) return 0;
        }
        else
        {
            if (wcsnicmp( path + start, redir, i - start )) return 0;
        }
        redir += i - start;
        while (i < len && IS_SEPARATOR(path[i])) i++;
        if (!*redir) return i;
        if (*redir++ != '\\') return 0;
    }
    return 0;
}


/***********************************************************************
 *           get_redirect_path
 *
 * Retrieve the Unix path corresponding to a redirected path if any.
 */
static int get_redirect_path( char *unix_name, int pos, const WCHAR *name, int length, BOOLEAN check_case )
{
    unsigned int i;
    int len;

    for (i = 0; i < nb_redirects; i++)
    {
        if ((len = match_redirect( name, length, redirects[i].source, check_case )))
        {
            if (!redirects[i].unix_target) break;
            unix_name[pos++] = '/';
            strcpy( unix_name + pos, redirects[i].unix_target );
            return len;
        }
    }
    return 0;
}

#else  /* _WIN64 */

/* there are no redirects on 64-bit */
static int get_redirect_path( char *unix_name, int pos, const WCHAR *name, int length, BOOLEAN check_case )
{
    return 0;
}

#endif

/***********************************************************************
 *           init_files
 */
void init_files(void)
{
#ifndef _WIN64
    if (is_wow64) init_redirects();
#endif
    /* a couple of directories that we don't want to return in directory searches */
    ignore_file( config_dir );
    ignore_file( "/dev" );
    ignore_file( "/proc" );
#ifdef linux
    ignore_file( "/sys" );
#endif
    /* retrieve initial umask */
    start_umask = umask( 0777 );
    umask( start_umask );
}


/******************************************************************************
 *           get_dos_device
 *
 * Get the Unix path of a DOS device.
 */
static NTSTATUS get_dos_device( const WCHAR *name, UINT name_len, char **unix_name_ret )
{
    struct stat st;
    char *unix_name, *new_name, *dev;
    unsigned int i;
    int unix_len;

    /* make sure the device name is ASCII */
    for (i = 0; i < name_len; i++)
        if (name[i] <= 32 || name[i] >= 127) return STATUS_BAD_DEVICE_TYPE;

    unix_len = strlen(config_dir) + sizeof("/dosdevices/") + name_len + 1;

    if (!(unix_name = malloc( unix_len ))) return STATUS_NO_MEMORY;

    strcpy( unix_name, config_dir );
    strcat( unix_name, "/dosdevices/" );
    dev = unix_name + strlen(unix_name);

    for (i = 0; i < name_len; i++) dev[i] = (name[i] >= 'A' && name[i] <= 'Z' ? name[i] + 32 : name[i]);
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
            *unix_name_ret = unix_name;
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

        new_name = NULL;
        if (dev[1] == ':' && dev[2] == ':')  /* drive device */
        {
            dev[2] = 0;  /* remove last ':' to get the drive mount point symlink */
            new_name = get_default_drive_device( unix_name );
        }

        if (!new_name) break;
        free( unix_name );
        unix_name = new_name;
        unix_len = strlen(unix_name) + 1;
        dev = NULL; /* last try */
    }
    free( unix_name );
    return STATUS_BAD_DEVICE_TYPE;
}


/* return the length of the DOS namespace prefix if any */
static inline int get_dos_prefix_len( const UNICODE_STRING *name )
{
    static const WCHAR nt_prefixW[] = {'\\','?','?','\\'};
    static const WCHAR dosdev_prefixW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\'};

    if (name->Length >= sizeof(nt_prefixW) &&
        !memcmp( name->Buffer, nt_prefixW, sizeof(nt_prefixW) ))
        return ARRAY_SIZE( nt_prefixW );

    if (name->Length >= sizeof(dosdev_prefixW) &&
        !wcsnicmp( name->Buffer, dosdev_prefixW, ARRAY_SIZE( dosdev_prefixW )))
        return ARRAY_SIZE( dosdev_prefixW );

    return 0;
}


/***********************************************************************
 *           remove_last_componentA
 *
 * Remove the last component of the path. Helper for find_drive_rootA.
 */
static inline unsigned int remove_last_componentA( const char *path, unsigned int len )
{
    int level = 0;

    while (level < 1)
    {
        /* find start of the last path component */
        unsigned int prev = len;
        if (prev <= 1) break;  /* reached root */
        while (prev > 1 && path[prev - 1] != '/') prev--;
        /* does removing it take us up a level? */
        if (len - prev != 1 || path[prev] != '.')  /* not '.' */
        {
            if (len - prev == 2 && path[prev] == '.' && path[prev+1] == '.')  /* is it '..'? */
                level--;
            else
                level++;
        }
        /* strip off trailing slashes */
        while (prev > 1 && path[prev - 1] == '/') prev--;
        len = prev;
    }
    return len;
}


/***********************************************************************
 *           find_drive_rootA
 *
 * Find a drive for which the root matches the beginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, ppath is modified
 * to point to the beginning of the DOS path.
 */
static NTSTATUS find_drive_rootA( LPCSTR *ppath, unsigned int len, int *drive_ret )
{
    /* Starting with the full path, check if the device and inode match any of
     * the wine 'drives'. If not then remove the last path component and try
     * again. If the last component was a '..' then skip a normal component
     * since it's a directory that's ascended back out of.
     */
    int drive;
    char *buffer;
    const char *path = *ppath;
    struct stat st;
    struct file_identity info[MAX_DOS_DRIVES];

    /* get device and inode of all drives */
    if (!get_drives_info( info )) return STATUS_OBJECT_PATH_NOT_FOUND;

    /* strip off trailing slashes */
    while (len > 1 && path[len - 1] == '/') len--;

    /* make a copy of the path */
    if (!(buffer = malloc( len + 1 ))) return STATUS_NO_MEMORY;
    memcpy( buffer, path, len );
    buffer[len] = 0;

    for (;;)
    {
        if (!stat( buffer, &st ) && S_ISDIR( st.st_mode ))
        {
            /* Find the drive */
            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
                if ((info[drive].dev == st.st_dev) && (info[drive].ino == st.st_ino))
                {
                    if (len == 1) len = 0;  /* preserve root slash in returned path */
                    TRACE( "%s -> drive %c:, root=%s, name=%s\n",
                           debugstr_a(path), 'A' + drive, debugstr_a(buffer), debugstr_a(path + len));
                    *ppath += len;
                    *drive_ret = drive;
                    free( buffer );
                    return STATUS_SUCCESS;
                }
            }
        }
        if (len <= 1) break;  /* reached root */
        len = remove_last_componentA( buffer, len );
        buffer[len] = 0;
    }
    free( buffer );
    return STATUS_OBJECT_PATH_NOT_FOUND;
}


/******************************************************************************
 *           find_file_id
 *
 * Recursively search directories from the dir queue for a given inode.
 */
static NTSTATUS find_file_id( char **unix_name, ULONG *len, ULONGLONG file_id, dev_t dev )
{
    unsigned int pos;
    DIR *dir;
    struct dirent *de;
    NTSTATUS status;
    struct stat st;
    char *name = *unix_name;

    while (!(status = next_dir_in_queue( name )))
    {
        if (!(dir = opendir( name ))) continue;
        TRACE( "searching %s for %s\n", debugstr_a(name), wine_dbgstr_longlong(file_id) );
        pos = strlen( name );
        if (pos + MAX_DIR_ENTRY_LEN >= *len / sizeof(WCHAR))
        {
            if (!(name = realloc( name, *len * 2 )))
            {
                closedir( dir );
                return STATUS_NO_MEMORY;
            }
            *len *= 2;
            *unix_name = name;
        }
        name[pos++] = '/';
        while ((de = readdir( dir )))
        {
            if (!strcmp( de->d_name, "." ) || !strcmp( de->d_name, ".." )) continue;
            strcpy( name + pos, de->d_name );
            if (lstat( name, &st ) == -1) continue;
            if (st.st_dev != dev) continue;
            if (st.st_ino == file_id)
            {
                closedir( dir );
                return STATUS_SUCCESS;
            }
            if (!S_ISDIR( st.st_mode )) continue;
            if ((status = add_dir_to_queue( name )) != STATUS_SUCCESS)
            {
                closedir( dir );
                return status;
            }
        }
        closedir( dir );
    }
    return status;
}


/******************************************************************************
 *           file_id_to_unix_file_name
 *
 * Lookup a file from its file id instead of its name.
 */
static NTSTATUS file_id_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char **unix_name_ret )
{
    enum server_fd_type type;
    int old_cwd, root_fd, needs_close;
    char *unix_name;
    ULONG len;
    NTSTATUS status;
    ULONGLONG file_id;
    struct stat st, root_st;

    if (attr->ObjectName->Length != sizeof(ULONGLONG)) return STATUS_OBJECT_PATH_SYNTAX_BAD;
    if (!attr->RootDirectory) return STATUS_INVALID_PARAMETER;
    memcpy( &file_id, attr->ObjectName->Buffer, sizeof(file_id) );

    len = 2 * MAX_DIR_ENTRY_LEN + 4;
    if (!(unix_name = malloc( len ))) return STATUS_NO_MEMORY;
    strcpy( unix_name, "." );

    if ((status = server_get_unix_fd( attr->RootDirectory, 0, &root_fd, &needs_close, &type, NULL )))
        goto done;

    if (type != FD_TYPE_DIR)
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto done;
    }

    fstat( root_fd, &root_st );
    if (root_st.st_ino == file_id)  /* shortcut for "." */
    {
        status = STATUS_SUCCESS;
        goto done;
    }

    mutex_lock( &dir_mutex );
    if ((old_cwd = open( ".", O_RDONLY )) != -1 && fchdir( root_fd ) != -1)
    {
        /* shortcut for ".." */
        if (!stat( "..", &st ) && st.st_dev == root_st.st_dev && st.st_ino == file_id)
        {
            strcpy( unix_name, ".." );
            status = STATUS_SUCCESS;
        }
        else
        {
            status = add_dir_to_queue( "." );
            if (!status)
                status = find_file_id( &unix_name, &len, file_id, root_st.st_dev );
            if (!status)  /* get rid of "./" prefix */
                memmove( unix_name, unix_name + 2, strlen(unix_name) - 1 );
            flush_dir_queue();
        }
        if (fchdir( old_cwd ) == -1) chdir( "/" );
    }
    else status = errno_to_status( errno );
    mutex_unlock( &dir_mutex );
    if (old_cwd != -1) close( old_cwd );

done:
    if (status == STATUS_SUCCESS)
    {
        TRACE( "%s -> %s\n", wine_dbgstr_longlong(file_id), debugstr_a(unix_name) );
        *unix_name_ret = unix_name;
    }
    else
    {
        TRACE( "%s not found in dir %p\n", wine_dbgstr_longlong(file_id), attr->RootDirectory );
        free( unix_name );
    }
    if (needs_close) close( root_fd );
    return status;
}


/******************************************************************************
 *           lookup_unix_name
 *
 * Helper for nt_to_unix_file_name
 */
static NTSTATUS lookup_unix_name( const WCHAR *name, int name_len, char **buffer, int unix_len, int pos,
                                  UINT disposition, BOOLEAN check_case )
{
    NTSTATUS status;
    int ret, len;
    struct stat st;
    char *unix_name = *buffer;
#ifdef _WIN64
    const BOOL redirect = FALSE;
#else
    const BOOL redirect = NtCurrentTeb64() && !NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR];
#endif

    /* try a shortcut first */

    while (name_len && IS_SEPARATOR(*name))
    {
        name++;
        name_len--;
    }

    unix_name[pos] = '/';
    ret = ntdll_wcstoumbs( name, name_len, unix_name + pos + 1, unix_len - pos - 1, TRUE );
    if (ret >= 0 && ret < unix_len - pos - 1)
    {
        char *p;
        unix_name[pos + 1 + ret] = 0;
        for (p = unix_name + pos ; *p; p++) if (*p == '\\') *p = '/';
        if (!name_len || !redirect || (!strstr( unix_name, "/windows/") && strncmp( unix_name, "windows/", 8 )))
        {
            if (!stat( unix_name, &st ))
            {
                if (disposition == FILE_CREATE)
                    return STATUS_OBJECT_NAME_COLLISION;
                return STATUS_SUCCESS;
            }
        }
    }

    if (!name_len)  /* empty name -> drive root doesn't exist */
        return STATUS_OBJECT_PATH_NOT_FOUND;
    if (check_case && !redirect && (disposition == FILE_OPEN || disposition == FILE_OVERWRITE))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    /* now do it component by component */

    while (name_len)
    {
        const WCHAR *end, *next;
        BOOLEAN is_win_dir = FALSE;

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
            if (!(new_name = realloc( unix_name, unix_len ))) return STATUS_NO_MEMORY;
            unix_name = *buffer = new_name;
        }

        status = find_file_in_dir( unix_name, pos, name, end - name,
                                   check_case, redirect ? &is_win_dir : NULL );

        /* if this is the last element, not finding it is not necessarily fatal */
        if (!name_len)
        {
            if (status == STATUS_OBJECT_PATH_NOT_FOUND)
            {
                status = STATUS_OBJECT_NAME_NOT_FOUND;
                if (disposition != FILE_OPEN && disposition != FILE_OVERWRITE)
                {
                    ret = ntdll_wcstoumbs( name, end - name, unix_name + pos + 1, MAX_DIR_ENTRY_LEN + 1, TRUE );
                    if (ret > 0 && ret <= MAX_DIR_ENTRY_LEN)
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

        if (status != STATUS_SUCCESS) break;

        pos += strlen( unix_name + pos );
        name = next;

        if (is_win_dir && (len = get_redirect_path( unix_name, pos, name, name_len, check_case )))
        {
            name += len;
            name_len -= len;
            pos += strlen( unix_name + pos );
            TRACE( "redirecting -> %s + %s\n", debugstr_a(unix_name), debugstr_w(name) );
        }
    }

    return status;
}


/******************************************************************************
 *           nt_to_unix_file_name_attr
 */
static NTSTATUS nt_to_unix_file_name_attr( const OBJECT_ATTRIBUTES *attr, char **name_ret,
                                           UINT disposition )
{
    static const WCHAR invalid_charsW[] = { INVALID_NT_CHARS, 0 };
    enum server_fd_type type;
    int old_cwd, root_fd, needs_close;
    const WCHAR *name, *p;
    char *unix_name;
    int name_len, unix_len;
    NTSTATUS status;

    if (!attr->RootDirectory)  /* without root dir fall back to normal lookup */
        return nt_to_unix_file_name( attr->ObjectName, name_ret, disposition );

    name     = attr->ObjectName->Buffer;
    name_len = attr->ObjectName->Length / sizeof(WCHAR);

    if (name_len && IS_SEPARATOR(name[0])) return STATUS_INVALID_PARAMETER;

    /* check for invalid characters */
    for (p = name; p < name + name_len; p++)
        if (*p < 32 || wcschr( invalid_charsW, *p )) return STATUS_OBJECT_NAME_INVALID;

    unix_len = name_len * 3 + MAX_DIR_ENTRY_LEN + 3;
    if (!(unix_name = malloc( unix_len ))) return STATUS_NO_MEMORY;
    unix_name[0] = '.';

    if (!(status = server_get_unix_fd( attr->RootDirectory, 0, &root_fd, &needs_close, &type, NULL )))
    {
        if (type != FD_TYPE_DIR)
        {
            if (needs_close) close( root_fd );
            status = STATUS_BAD_DEVICE_TYPE;
        }
        else
        {
            mutex_lock( &dir_mutex );
            if ((old_cwd = open( ".", O_RDONLY )) != -1 && fchdir( root_fd ) != -1)
            {
                status = lookup_unix_name( name, name_len, &unix_name, unix_len, 1,
                                           disposition, FALSE );
                if (fchdir( old_cwd ) == -1) chdir( "/" );
            }
            else status = errno_to_status( errno );
            mutex_unlock( &dir_mutex );
            if (old_cwd != -1) close( old_cwd );
            if (needs_close) close( root_fd );
        }
    }
    else if (status == STATUS_OBJECT_TYPE_MISMATCH) status = STATUS_BAD_DEVICE_TYPE;

    if (status == STATUS_SUCCESS || status == STATUS_NO_SUCH_FILE)
    {
        TRACE( "%s -> %s\n", debugstr_us(attr->ObjectName), debugstr_a(unix_name) );
        *name_ret = unix_name;
    }
    else
    {
        TRACE( "%s not found in %s\n", debugstr_w(name), unix_name );
        free( unix_name );
    }
    return status;
}


/******************************************************************************
 *           nt_to_unix_file_name
 *
 * Convert a file name from NT namespace to Unix namespace.
 *
 * If disposition is not FILE_OPEN or FILE_OVERWRITE, the last path
 * element doesn't have to exist; in that case STATUS_NO_SUCH_FILE is
 * returned, but the unix name is still filled in properly.
 */
NTSTATUS nt_to_unix_file_name( const UNICODE_STRING *nameW, char **unix_name_ret, UINT disposition )
{
    static const WCHAR unixW[] = {'u','n','i','x'};
    static const WCHAR invalid_charsW[] = { INVALID_NT_CHARS, 0 };

    NTSTATUS status = STATUS_SUCCESS;
    const WCHAR *name, *p;
    struct stat st;
    char *unix_name;
    int pos, ret, name_len, unix_len, prefix_len;
    WCHAR prefix[MAX_DIR_ENTRY_LEN + 1];
    BOOLEAN check_case = FALSE;
    BOOLEAN is_unix = FALSE;

    name     = nameW->Buffer;
    name_len = nameW->Length / sizeof(WCHAR);

    if (!name_len || !IS_SEPARATOR(name[0])) return STATUS_OBJECT_PATH_SYNTAX_BAD;

    if (!(pos = get_dos_prefix_len( nameW )))
        return STATUS_BAD_DEVICE_TYPE;  /* no DOS prefix, assume NT native name */

    name += pos;
    name_len -= pos;

    if (!name_len) return STATUS_OBJECT_NAME_INVALID;

    /* check for sub-directory */
    for (pos = 0; pos < name_len && pos <= MAX_DIR_ENTRY_LEN; pos++)
    {
        if (IS_SEPARATOR(name[pos])) break;
        if (name[pos] < 32 || wcschr( invalid_charsW, name[pos] ))
            return STATUS_OBJECT_NAME_INVALID;
        prefix[pos] = (name[pos] >= 'A' && name[pos] <= 'Z') ? name[pos] + 'a' - 'A' : name[pos];
    }
    if (pos > MAX_DIR_ENTRY_LEN) return STATUS_OBJECT_NAME_INVALID;

    if (pos == name_len)  /* no subdir, plain DOS device */
        return get_dos_device( name, name_len, unix_name_ret );

    prefix_len = pos;
    prefix[prefix_len] = 0;

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
            if (*p < 32 || wcschr( invalid_charsW, *p )) return STATUS_OBJECT_NAME_INVALID;
    }

    unix_len = (prefix_len + name_len) * 3 + MAX_DIR_ENTRY_LEN + 3;
    unix_len += strlen(config_dir) + sizeof("/dosdevices/");
    if (!(unix_name = malloc( unix_len ))) return STATUS_NO_MEMORY;
    strcpy( unix_name, config_dir );
    strcat( unix_name, "/dosdevices/" );
    pos = strlen(unix_name);

    ret = ntdll_wcstoumbs( prefix, prefix_len, unix_name + pos, unix_len - pos - 1, TRUE );
    if (ret <= 0)
    {
        free( unix_name );
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
                free( unix_name );
                return STATUS_BAD_DEVICE_TYPE;
            }
            pos = 0;  /* fall back to unix root */
        }
    }

    status = lookup_unix_name( name, name_len, &unix_name, unix_len, pos, disposition, check_case );
    if (status == STATUS_SUCCESS || status == STATUS_NO_SUCH_FILE)
    {
        TRACE( "%s -> %s\n", debugstr_us(nameW), debugstr_a(unix_name) );
        *unix_name_ret = unix_name;
    }
    else
    {
        TRACE( "%s not found in %s\n", debugstr_w(name), unix_name );
        free( unix_name );
    }
    return status;
}


/******************************************************************************
 *           wine_nt_to_unix_file_name
 *
 * Convert a file name from NT namespace to Unix namespace.
 *
 * If disposition is not FILE_OPEN or FILE_OVERWRITE, the last path
 * element doesn't have to exist; in that case STATUS_NO_SUCH_FILE is
 * returned, but the unix name is still filled in properly.
 */
NTSTATUS CDECL wine_nt_to_unix_file_name( const UNICODE_STRING *nameW, char *nameA, SIZE_T *size,
                                          UINT disposition )
{
    char *buffer = NULL;
    NTSTATUS status = nt_to_unix_file_name( nameW, &buffer, disposition );

    if (buffer)
    {
        if (*size > strlen(buffer)) strcpy( nameA, buffer );
        else status = STATUS_BUFFER_TOO_SMALL;
        *size = strlen(buffer) + 1;
        free( buffer );
    }
    return status;
}


/******************************************************************
 *           unix_to_nt_file_name
 */
NTSTATUS unix_to_nt_file_name( const char *name, WCHAR **nt )
{
    static const WCHAR unix_prefixW[] = {'\\','?','?','\\','u','n','i','x',0};
    WCHAR dos_prefixW[] = {'\\','?','?','\\','A',':','\\',0};
    const WCHAR *prefix = unix_prefixW;
    unsigned int lenW, lenA = strlen(name);
    const char *path = name;
    NTSTATUS status;
    WCHAR *p, *buffer;
    int drive;

    status = find_drive_rootA( &path, lenA, &drive );
    lenA -= path - name;

    if (status == STATUS_SUCCESS)
    {
        while (lenA && path[0] == '/') { lenA--; path++; }
        dos_prefixW[4] += drive;
        prefix = dos_prefixW;
    }
    else if (status != STATUS_OBJECT_PATH_NOT_FOUND) return status;

    lenW = wcslen( prefix );
    if (!(buffer = malloc( (lenA + lenW + 1) * sizeof(WCHAR) ))) return STATUS_NO_MEMORY;
    memcpy( buffer, prefix, lenW * sizeof(WCHAR) );
    lenW += ntdll_umbstowcs( path, lenA, buffer + lenW, lenA );
    buffer[lenW] = 0;
    for (p = buffer; *p; p++) if (*p == '/') *p = '\\';
    *nt = buffer;
    return STATUS_SUCCESS;
}


/******************************************************************
 *           wine_unix_to_nt_file_name
 */
NTSTATUS CDECL wine_unix_to_nt_file_name( const char *name, WCHAR *buffer, SIZE_T *size )
{
    WCHAR *nt_name = NULL;
    NTSTATUS status;

    if (name[0] != '/') return STATUS_INVALID_PARAMETER;  /* relative paths are not supported */

    status = unix_to_nt_file_name( name, &nt_name );
    if (nt_name)
    {
        if (*size > wcslen(nt_name)) wcscpy( buffer, nt_name );
        else status = STATUS_BUFFER_TOO_SMALL;
        *size = wcslen(nt_name) + 1;
        free( nt_name );
    }
    return status;
}


/***********************************************************************
 *           unmount_device
 *
 * Unmount the specified device.
 */
static NTSTATUS unmount_device( HANDLE handle )
{
    NTSTATUS status;
    int unix_fd, needs_close;

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
                char *cmd = malloc( strlen(mount_point)+sizeof(umount));
                if (cmd)
                {
                    strcpy( cmd, umount );
                    strcat( cmd, mount_point );
                    system( cmd );
                    free( cmd );
#ifdef linux
                    /* umount will fail to release the loop device since we still have
                       a handle to it, so we release it here */
                    if (major(st.st_rdev) == LOOP_MAJOR) ioctl( unix_fd, 0x4c01 /*LOOP_CLR_FD*/, 0 );
#endif
                }
                free( mount_point );
            }
        }
        if (needs_close) close( unix_fd );
    }
    return status;
}


/***********************************************************************
 *           set_show_dot_files
 */
void CDECL set_show_dot_files( BOOL enable )
{
    show_dot_files = enable;
}


/******************************************************************************
 *              open_unix_file
 *
 * Helper for NtCreateFile that takes a Unix path.
 */
NTSTATUS open_unix_file( HANDLE *handle, const char *unix_name, ACCESS_MASK access,
                         OBJECT_ATTRIBUTES *attr, ULONG attributes, ULONG sharing, ULONG disposition,
                         ULONG options, void *ea_buffer, ULONG ea_length )
{
    static UNICODE_STRING empty_string;
    struct object_attributes *objattr;
    OBJECT_ATTRIBUTES unix_attr = *attr;
    NTSTATUS status;
    data_size_t len;

    unix_attr.ObjectName = &empty_string;  /* we send the unix name instead */
    if ((status = alloc_object_attributes( &unix_attr, &objattr, &len ))) return status;

    SERVER_START_REQ( create_file )
    {
        req->access     = access;
        req->sharing    = sharing;
        req->create     = disposition;
        req->options    = options;
        req->attrs      = attributes;
        wine_server_add_data( req, objattr, len );
        wine_server_add_data( req, unix_name, strlen(unix_name) );
        status = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    free( objattr );
    return status;
}


/******************************************************************************
 *              NtCreateFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateFile( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                              IO_STATUS_BLOCK *io, LARGE_INTEGER *alloc_size,
                              ULONG attributes, ULONG sharing, ULONG disposition,
                              ULONG options, void *ea_buffer, ULONG ea_length )
{
    char *unix_name;
    BOOL created = FALSE;

    TRACE( "handle=%p access=%08x name=%s objattr=%08x root=%p sec=%p io=%p alloc_size=%p "
           "attr=%08x sharing=%08x disp=%d options=%08x ea=%p.0x%08x\n",
           handle, access, debugstr_us(attr->ObjectName), attr->Attributes,
           attr->RootDirectory, attr->SecurityDescriptor, io, alloc_size,
           attributes, sharing, disposition, options, ea_buffer, ea_length );

    if (!attr || !attr->ObjectName) return STATUS_INVALID_PARAMETER;

    if (alloc_size) FIXME( "alloc_size not supported\n" );

    if (options & FILE_OPEN_BY_FILE_ID)
        io->u.Status = file_id_to_unix_file_name( attr, &unix_name );
    else
        io->u.Status = nt_to_unix_file_name_attr( attr, &unix_name, disposition );

    if (io->u.Status == STATUS_BAD_DEVICE_TYPE)
    {
        SERVER_START_REQ( open_file_object )
        {
            req->access     = access;
            req->attributes = attr->Attributes;
            req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
            req->sharing    = sharing;
            req->options    = options;
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
            io->u.Status = wine_server_call( req );
            *handle = wine_server_ptr_handle( reply->handle );
        }
        SERVER_END_REQ;
        if (io->u.Status == STATUS_SUCCESS) io->Information = FILE_OPENED;
        return io->u.Status;
    }

    if (io->u.Status == STATUS_NO_SUCH_FILE && disposition != FILE_OPEN && disposition != FILE_OVERWRITE)
    {
        created = TRUE;
        io->u.Status = STATUS_SUCCESS;
    }

    if (io->u.Status == STATUS_SUCCESS)
    {
        io->u.Status = open_unix_file( handle, unix_name, access, attr, attributes,
                                       sharing, disposition, options, ea_buffer, ea_length );
        free( unix_name );
    }
    else WARN( "%s not found (%x)\n", debugstr_us(attr->ObjectName), io->u.Status );

    if (io->u.Status == STATUS_SUCCESS)
    {
        if (created) io->Information = FILE_CREATED;
        else switch(disposition)
        {
        case FILE_SUPERSEDE:
            io->Information = FILE_SUPERSEDED;
            break;
        case FILE_CREATE:
            io->Information = FILE_CREATED;
            break;
        case FILE_OPEN:
        case FILE_OPEN_IF:
            io->Information = FILE_OPENED;
            break;
        case FILE_OVERWRITE:
        case FILE_OVERWRITE_IF:
            io->Information = FILE_OVERWRITTEN;
            break;
        }
    }
    else if (io->u.Status == STATUS_TOO_MANY_OPENED_FILES)
    {
        static int once;
        if (!once++) ERR_(winediag)( "Too many open files, ulimit -n probably needs to be increased\n" );
    }

    return io->u.Status;
}


/******************************************************************************
 *              NtOpenFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenFile( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                            IO_STATUS_BLOCK *io, ULONG sharing, ULONG options )
{
    return NtCreateFile( handle, access, attr, io, NULL, 0, sharing, FILE_OPEN, options, NULL, 0 );
}


/******************************************************************************
 *		NtCreateMailslotFile    (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateMailslotFile( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                      IO_STATUS_BLOCK *io, ULONG options, ULONG quota, ULONG msg_size,
                                      LARGE_INTEGER *timeout )
{
    NTSTATUS status;
    data_size_t len;
    struct object_attributes *objattr;

    TRACE( "%p %08x %p %p %08x %08x %08x %p\n",
           handle, access, attr, io, options, quota, msg_size, timeout );

    if (!handle) return STATUS_ACCESS_VIOLATION;
    if (!attr) return STATUS_INVALID_PARAMETER;

    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

    SERVER_START_REQ( create_mailslot )
    {
        req->access       = access;
        req->max_msgsize  = msg_size;
        req->read_timeout = timeout ? timeout->QuadPart : -1;
        wine_server_add_data( req, objattr, len );
        if (!(status = wine_server_call( req ))) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    free( objattr );
    return status;
}


/******************************************************************
 *		NtCreateNamedPipeFile    (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateNamedPipeFile( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                       IO_STATUS_BLOCK *io, ULONG sharing, ULONG dispo, ULONG options,
                                       ULONG pipe_type, ULONG read_mode, ULONG completion_mode,
                                       ULONG max_inst, ULONG inbound_quota, ULONG outbound_quota,
                                       LARGE_INTEGER *timeout )
{
    NTSTATUS status;
    data_size_t len;
    struct object_attributes *objattr;

    if (!attr) return STATUS_INVALID_PARAMETER;

    TRACE( "(%p %x %s %p %x %d %x %d %d %d %d %d %d %p)\n",
           handle, access, debugstr_us(attr->ObjectName), io, sharing, dispo,
           options, pipe_type, read_mode, completion_mode, max_inst, inbound_quota,
           outbound_quota, timeout );

    /* assume we only get relative timeout */
    if (timeout->QuadPart > 0) FIXME( "Wrong time %s\n", wine_dbgstr_longlong(timeout->QuadPart) );

    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

    SERVER_START_REQ( create_named_pipe )
    {
        req->access  = access;
        req->options = options;
        req->sharing = sharing;
        req->flags =
            (pipe_type ? NAMED_PIPE_MESSAGE_STREAM_WRITE   : 0) |
            (read_mode ? NAMED_PIPE_MESSAGE_STREAM_READ    : 0) |
            (completion_mode ? NAMED_PIPE_NONBLOCKING_MODE : 0);
        req->maxinstances = max_inst;
        req->outsize = outbound_quota;
        req->insize  = inbound_quota;
        req->timeout = timeout->QuadPart;
        wine_server_add_data( req, objattr, len );
        if (!(status = wine_server_call( req ))) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    free( objattr );
    return status;
}


/******************************************************************
 *              NtDeleteFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtDeleteFile( OBJECT_ATTRIBUTES *attr )
{
    HANDLE handle;
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    status = NtCreateFile( &handle, GENERIC_READ | GENERIC_WRITE | DELETE, attr, &io, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
                           FILE_DELETE_ON_CLOSE, NULL, 0 );
    if (status == STATUS_SUCCESS) NtClose( handle );
    return status;
}


/******************************************************************************
 *              NtQueryFullAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryFullAttributesFile( const OBJECT_ATTRIBUTES *attr,
                                           FILE_NETWORK_OPEN_INFORMATION *info )
{
    char *unix_name;
    NTSTATUS status;

    if (!(status = nt_to_unix_file_name_attr( attr, &unix_name, FILE_OPEN )))
    {
        ULONG attributes;
        struct stat st;

        if (get_file_info( unix_name, &st, &attributes ) == -1)
            status = errno_to_status( errno );
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
        {
            FILE_BASIC_INFORMATION basic;
            FILE_STANDARD_INFORMATION std;

            fill_file_info( &st, attributes, &basic, FileBasicInformation );
            fill_file_info( &st, attributes, &std, FileStandardInformation );

            info->CreationTime   = basic.CreationTime;
            info->LastAccessTime = basic.LastAccessTime;
            info->LastWriteTime  = basic.LastWriteTime;
            info->ChangeTime     = basic.ChangeTime;
            info->AllocationSize = std.AllocationSize;
            info->EndOfFile      = std.EndOfFile;
            info->FileAttributes = basic.FileAttributes;
            if (is_hidden_file( attr->ObjectName )) info->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        }
        free( unix_name );
    }
    else WARN( "%s not found (%x)\n", debugstr_us(attr->ObjectName), status );
    return status;
}


/******************************************************************************
 *              NtQueryAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryAttributesFile( const OBJECT_ATTRIBUTES *attr, FILE_BASIC_INFORMATION *info )
{
    char *unix_name;
    NTSTATUS status;

    if (!(status = nt_to_unix_file_name_attr( attr, &unix_name, FILE_OPEN )))
    {
        ULONG attributes;
        struct stat st;

        if (get_file_info( unix_name, &st, &attributes ) == -1)
            status = errno_to_status( errno );
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
        {
            status = fill_file_info( &st, attributes, info, FileBasicInformation );
            if (is_hidden_file( attr->ObjectName )) info->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        }
        free( unix_name );
    }
    else WARN( "%s not found (%x)\n", debugstr_us(attr->ObjectName), status );
    return status;
}


/******************************************************************************
 *              NtQueryInformationFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                        void *ptr, LONG len, FILE_INFORMATION_CLASS class )
{
    static const size_t info_sizes[] =
    {
        0,
        sizeof(FILE_DIRECTORY_INFORMATION),            /* FileDirectoryInformation */
        sizeof(FILE_FULL_DIRECTORY_INFORMATION),       /* FileFullDirectoryInformation */
        sizeof(FILE_BOTH_DIRECTORY_INFORMATION),       /* FileBothDirectoryInformation */
        sizeof(FILE_BASIC_INFORMATION),                /* FileBasicInformation */
        sizeof(FILE_STANDARD_INFORMATION),             /* FileStandardInformation */
        sizeof(FILE_INTERNAL_INFORMATION),             /* FileInternalInformation */
        sizeof(FILE_EA_INFORMATION),                   /* FileEaInformation */
        0,                                             /* FileAccessInformation */
        sizeof(FILE_NAME_INFORMATION),                 /* FileNameInformation */
        sizeof(FILE_RENAME_INFORMATION)-sizeof(WCHAR), /* FileRenameInformation */
        0,                                             /* FileLinkInformation */
        sizeof(FILE_NAMES_INFORMATION)-sizeof(WCHAR),  /* FileNamesInformation */
        sizeof(FILE_DISPOSITION_INFORMATION),          /* FileDispositionInformation */
        sizeof(FILE_POSITION_INFORMATION),             /* FilePositionInformation */
        sizeof(FILE_FULL_EA_INFORMATION),              /* FileFullEaInformation */
        0,                                             /* FileModeInformation */
        sizeof(FILE_ALIGNMENT_INFORMATION),            /* FileAlignmentInformation */
        sizeof(FILE_ALL_INFORMATION),                  /* FileAllInformation */
        sizeof(FILE_ALLOCATION_INFORMATION),           /* FileAllocationInformation */
        sizeof(FILE_END_OF_FILE_INFORMATION),          /* FileEndOfFileInformation */
        0,                                             /* FileAlternateNameInformation */
        sizeof(FILE_STREAM_INFORMATION)-sizeof(WCHAR), /* FileStreamInformation */
        sizeof(FILE_PIPE_INFORMATION),                 /* FilePipeInformation */
        sizeof(FILE_PIPE_LOCAL_INFORMATION),           /* FilePipeLocalInformation */
        0,                                             /* FilePipeRemoteInformation */
        sizeof(FILE_MAILSLOT_QUERY_INFORMATION),       /* FileMailslotQueryInformation */
        0,                                             /* FileMailslotSetInformation */
        0,                                             /* FileCompressionInformation */
        0,                                             /* FileObjectIdInformation */
        0,                                             /* FileCompletionInformation */
        0,                                             /* FileMoveClusterInformation */
        0,                                             /* FileQuotaInformation */
        0,                                             /* FileReparsePointInformation */
        sizeof(FILE_NETWORK_OPEN_INFORMATION),         /* FileNetworkOpenInformation */
        sizeof(FILE_ATTRIBUTE_TAG_INFORMATION),        /* FileAttributeTagInformation */
        0,                                             /* FileTrackingInformation */
        0,                                             /* FileIdBothDirectoryInformation */
        0,                                             /* FileIdFullDirectoryInformation */
        0,                                             /* FileValidDataLengthInformation */
        0,                                             /* FileShortNameInformation */
        0,                                             /* FileIoCompletionNotificationInformation, */
        0,                                             /* FileIoStatusBlockRangeInformation */
        0,                                             /* FileIoPriorityHintInformation */
        0,                                             /* FileSfioReserveInformation */
        0,                                             /* FileSfioVolumeInformation */
        0,                                             /* FileHardLinkInformation */
        0,                                             /* FileProcessIdsUsingFileInformation */
        0,                                             /* FileNormalizedNameInformation */
        0,                                             /* FileNetworkPhysicalNameInformation */
        0,                                             /* FileIdGlobalTxDirectoryInformation */
        0,                                             /* FileIsRemoteDeviceInformation */
        0,                                             /* FileAttributeCacheInformation */
        0,                                             /* FileNumaNodeInformation */
        0,                                             /* FileStandardLinkInformation */
        0,                                             /* FileRemoteProtocolInformation */
        0,                                             /* FileRenameInformationBypassAccessCheck */
        0,                                             /* FileLinkInformationBypassAccessCheck */
        0,                                             /* FileVolumeNameInformation */
        sizeof(FILE_ID_INFORMATION),                   /* FileIdInformation */
        0,                                             /* FileIdExtdDirectoryInformation */
        0,                                             /* FileReplaceCompletionInformation */
        0,                                             /* FileHardLinkFullIdInformation */
        0,                                             /* FileIdExtdBothDirectoryInformation */
    };

    struct stat st;
    int fd, needs_close = FALSE;
    ULONG attr;
    unsigned int options;

    TRACE( "(%p,%p,%p,0x%08x,0x%08x)\n", handle, io, ptr, len, class);

    io->Information = 0;

    if (class <= 0 || class >= FileMaximumInformation)
        return io->u.Status = STATUS_INVALID_INFO_CLASS;
    if (!info_sizes[class])
        return server_get_file_info( handle, io, ptr, len, class );
    if (len < info_sizes[class])
        return io->u.Status = STATUS_INFO_LENGTH_MISMATCH;

    if ((io->u.Status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, &options )))
    {
        if (io->u.Status != STATUS_BAD_DEVICE_TYPE) return io->u.Status;
        return server_get_file_info( handle, io, ptr, len, class );
    }

    switch (class)
    {
    case FileBasicInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1)
            io->u.Status = errno_to_status( errno );
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            io->u.Status = STATUS_INVALID_INFO_CLASS;
        else
            fill_file_info( &st, attr, ptr, class );
        break;
    case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION *info = ptr;

            if (fd_get_file_info( fd, options, &st, &attr ) == -1) io->u.Status = errno_to_status( errno );
            else
            {
                fill_file_info( &st, attr, info, class );
                info->DeletePending = FALSE; /* FIXME */
            }
        }
        break;
    case FilePositionInformation:
        {
            FILE_POSITION_INFORMATION *info = ptr;
            off_t res = lseek( fd, 0, SEEK_CUR );
            if (res == (off_t)-1) io->u.Status = errno_to_status( errno );
            else info->CurrentByteOffset.QuadPart = res;
        }
        break;
    case FileInternalInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) io->u.Status = errno_to_status( errno );
        else fill_file_info( &st, attr, ptr, class );
        break;
    case FileEaInformation:
        {
            FILE_EA_INFORMATION *info = ptr;
            info->EaSize = 0;
        }
        break;
    case FileEndOfFileInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) io->u.Status = errno_to_status( errno );
        else fill_file_info( &st, attr, ptr, class );
        break;
    case FileAllInformation:
        {
            FILE_ALL_INFORMATION *info = ptr;
            char *unix_name;

            if (fd_get_file_info( fd, options, &st, &attr ) == -1) io->u.Status = errno_to_status( errno );
            else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
                io->u.Status = STATUS_INVALID_INFO_CLASS;
            else if (!(io->u.Status = server_get_unix_name( handle, &unix_name )))
            {
                LONG name_len = len - FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName);

                fill_file_info( &st, attr, info, FileAllInformation );
                info->StandardInformation.DeletePending = FALSE; /* FIXME */
                info->EaInformation.EaSize = 0;
                info->AccessInformation.AccessFlags = 0;  /* FIXME */
                info->PositionInformation.CurrentByteOffset.QuadPart = lseek( fd, 0, SEEK_CUR );
                info->ModeInformation.Mode = 0;  /* FIXME */
                info->AlignmentInformation.AlignmentRequirement = 1;  /* FIXME */

                io->u.Status = fill_name_info( unix_name, &info->NameInformation, &name_len );
                free( unix_name );
                io->Information = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + name_len;
            }
        }
        break;
    case FileMailslotQueryInformation:
        {
            FILE_MAILSLOT_QUERY_INFORMATION *info = ptr;

            SERVER_START_REQ( set_mailslot_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags = 0;
                io->u.Status = wine_server_call( req );
                if( io->u.Status == STATUS_SUCCESS )
                {
                    info->MaximumMessageSize = reply->max_msgsize;
                    info->MailslotQuota = 0;
                    info->NextMessageSize = 0;
                    info->MessagesAvailable = 0;
                    info->ReadTimeout.QuadPart = reply->read_timeout;
                }
            }
            SERVER_END_REQ;
            if (!io->u.Status)
            {
                char *tmpbuf;
                ULONG size = info->MaximumMessageSize ? info->MaximumMessageSize : 0x10000;
                if (size > 0x10000) size = 0x10000;
                if ((tmpbuf = malloc( size )))
                {
                    if (!server_get_unix_fd( handle, FILE_READ_DATA, &fd, &needs_close, NULL, NULL ))
                    {
                        int res = recv( fd, tmpbuf, size, MSG_PEEK );
                        info->MessagesAvailable = (res > 0);
                        info->NextMessageSize = (res >= 0) ? res : MAILSLOT_NO_MESSAGE;
                        if (needs_close) close( fd );
                    }
                    free( tmpbuf );
                }
            }
        }
        break;
    case FileNameInformation:
        {
            FILE_NAME_INFORMATION *info = ptr;
            char *unix_name;

            if (!(io->u.Status = server_get_unix_name( handle, &unix_name )))
            {
                LONG name_len = len - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
                io->u.Status = fill_name_info( unix_name, info, &name_len );
                free( unix_name );
                io->Information = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) + name_len;
            }
        }
        break;
    case FileNetworkOpenInformation:
        {
            FILE_NETWORK_OPEN_INFORMATION *info = ptr;
            char *unix_name;

            if (!(io->u.Status = server_get_unix_name( handle, &unix_name )))
            {
                ULONG attributes;
                struct stat st;

                if (get_file_info( unix_name, &st, &attributes ) == -1)
                    io->u.Status = errno_to_status( errno );
                else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
                    io->u.Status = STATUS_INVALID_INFO_CLASS;
                else
                {
                    FILE_BASIC_INFORMATION basic;
                    FILE_STANDARD_INFORMATION std;

                    fill_file_info( &st, attributes, &basic, FileBasicInformation );
                    fill_file_info( &st, attributes, &std, FileStandardInformation );

                    info->CreationTime   = basic.CreationTime;
                    info->LastAccessTime = basic.LastAccessTime;
                    info->LastWriteTime  = basic.LastWriteTime;
                    info->ChangeTime     = basic.ChangeTime;
                    info->AllocationSize = std.AllocationSize;
                    info->EndOfFile      = std.EndOfFile;
                    info->FileAttributes = basic.FileAttributes;
                }
                free( unix_name );
            }
        }
        break;
    case FileIdInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) io->u.Status = errno_to_status( errno );
        else
        {
            struct mountmgr_unix_drive drive;
            FILE_ID_INFORMATION *info = ptr;

            info->VolumeSerialNumber = 0;
            if (!(io->u.Status = get_mountmgr_fs_info( handle, fd, &drive, sizeof(drive) )))
                info->VolumeSerialNumber = drive.serial;
            memset( &info->FileId, 0, sizeof(info->FileId) );
            *(ULONGLONG *)&info->FileId = st.st_ino;
        }
        break;
    case FileAttributeTagInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) io->u.Status = errno_to_status( errno );
        else
        {
            FILE_ATTRIBUTE_TAG_INFORMATION *info = ptr;
            info->FileAttributes = attr;
            info->ReparseTag = 0; /* FIXME */
            if ((options & FILE_OPEN_REPARSE_POINT) && fd_is_mount_point( fd, &st ))
                info->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        }
        break;
    default:
        FIXME("Unsupported class (%d)\n", class);
        io->u.Status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    if (needs_close) close( fd );
    if (io->u.Status == STATUS_SUCCESS && !io->Information) io->Information = info_sizes[class];
    return io->u.Status;
}


/******************************************************************************
 *              NtSetInformationFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                      void *ptr, ULONG len, FILE_INFORMATION_CLASS class )
{
    int fd, needs_close;

    TRACE( "(%p,%p,%p,0x%08x,0x%08x)\n", handle, io, ptr, len, class );

    io->u.Status = STATUS_SUCCESS;
    switch (class)
    {
    case FileBasicInformation:
        if (len >= sizeof(FILE_BASIC_INFORMATION))
        {
            struct stat st;
            const FILE_BASIC_INFORMATION *info = ptr;
            LARGE_INTEGER mtime, atime;

            if ((io->u.Status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return io->u.Status;

            mtime.QuadPart = info->LastWriteTime.QuadPart == -1 ? 0 : info->LastWriteTime.QuadPart;
            atime.QuadPart = info->LastAccessTime.QuadPart == -1 ? 0 : info->LastAccessTime.QuadPart;

            if (atime.QuadPart || mtime.QuadPart)
                io->u.Status = set_file_times( fd, &mtime, &atime );

            if (io->u.Status == STATUS_SUCCESS && info->FileAttributes)
            {
                if (fstat( fd, &st ) == -1) io->u.Status = errno_to_status( errno );
                else
                {
                    if (info->FileAttributes & FILE_ATTRIBUTE_READONLY)
                    {
                        if (S_ISDIR( st.st_mode))
                            WARN("FILE_ATTRIBUTE_READONLY ignored for directory.\n");
                        else
                            st.st_mode &= ~0222; /* clear write permission bits */
                    }
                    else
                    {
                        /* add write permission only where we already have read permission */
                        st.st_mode |= (0600 | ((st.st_mode & 044) >> 1)) & (~start_umask);
                    }
                    if (fchmod( fd, st.st_mode ) == -1) io->u.Status = errno_to_status( errno );
                }
            }

            if (needs_close) close( fd );
        }
        else io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FilePositionInformation:
        if (len >= sizeof(FILE_POSITION_INFORMATION))
        {
            const FILE_POSITION_INFORMATION *info = ptr;

            if ((io->u.Status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return io->u.Status;

            if (lseek( fd, info->CurrentByteOffset.QuadPart, SEEK_SET ) == (off_t)-1)
                io->u.Status = errno_to_status( errno );

            if (needs_close) close( fd );
        }
        else io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileEndOfFileInformation:
        if (len >= sizeof(FILE_END_OF_FILE_INFORMATION))
        {
            struct stat st;
            const FILE_END_OF_FILE_INFORMATION *info = ptr;

            if ((io->u.Status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return io->u.Status;

            /* first try normal truncate */
            if (ftruncate( fd, (off_t)info->EndOfFile.QuadPart ) != -1) break;

            /* now check for the need to extend the file */
            if (fstat( fd, &st ) != -1 && (off_t)info->EndOfFile.QuadPart > st.st_size)
            {
                static const char zero;

                /* extend the file one byte beyond the requested size and then truncate it */
                /* this should work around ftruncate implementations that can't extend files */
                if (pwrite( fd, &zero, 1, (off_t)info->EndOfFile.QuadPart ) != -1 &&
                    ftruncate( fd, (off_t)info->EndOfFile.QuadPart ) != -1) break;
            }
            io->u.Status = errno_to_status( errno );

            if (needs_close) close( fd );
        }
        else io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FilePipeInformation:
        if (len >= sizeof(FILE_PIPE_INFORMATION))
        {
            FILE_PIPE_INFORMATION *info = ptr;

            if ((info->CompletionMode | info->ReadMode) & ~1)
            {
                io->u.Status = STATUS_INVALID_PARAMETER;
                break;
            }

            SERVER_START_REQ( set_named_pipe_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags  = (info->CompletionMode ? NAMED_PIPE_NONBLOCKING_MODE    : 0) |
                              (info->ReadMode       ? NAMED_PIPE_MESSAGE_STREAM_READ : 0);
                io->u.Status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        else io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileMailslotSetInformation:
        {
            FILE_MAILSLOT_SET_INFORMATION *info = ptr;

            SERVER_START_REQ( set_mailslot_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags = MAILSLOT_SET_READ_TIMEOUT;
                req->read_timeout = info->ReadTimeout.QuadPart;
                io->u.Status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        break;

    case FileCompletionInformation:
        if (len >= sizeof(FILE_COMPLETION_INFORMATION))
        {
            FILE_COMPLETION_INFORMATION *info = ptr;

            SERVER_START_REQ( set_completion_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->chandle  = wine_server_obj_handle( info->CompletionPort );
                req->ckey     = info->CompletionKey;
                io->u.Status  = wine_server_call( req );
            }
            SERVER_END_REQ;
        } else
            io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileIoCompletionNotificationInformation:
        if (len >= sizeof(FILE_IO_COMPLETION_NOTIFICATION_INFORMATION))
        {
            FILE_IO_COMPLETION_NOTIFICATION_INFORMATION *info = ptr;

            if (info->Flags & FILE_SKIP_SET_USER_EVENT_ON_FAST_IO)
                FIXME( "FILE_SKIP_SET_USER_EVENT_ON_FAST_IO not supported\n" );

            SERVER_START_REQ( set_fd_completion_mode )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->flags    = info->Flags;
                io->u.Status  = wine_server_call( req );
            }
            SERVER_END_REQ;
        } else
            io->u.Status = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case FileIoPriorityHintInformation:
        if (len >= sizeof(FILE_IO_PRIORITY_HINT_INFO))
        {
            FILE_IO_PRIORITY_HINT_INFO *info = ptr;
            if (info->PriorityHint < MaximumIoPriorityHintType)
                TRACE( "ignoring FileIoPriorityHintInformation %u\n", info->PriorityHint );
            else
                io->u.Status = STATUS_INVALID_PARAMETER;
        }
        else io->u.Status = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case FileAllInformation:
        io->u.Status = STATUS_INVALID_INFO_CLASS;
        break;

    case FileValidDataLengthInformation:
        if (len >= sizeof(FILE_VALID_DATA_LENGTH_INFORMATION))
        {
            struct stat st;
            const FILE_VALID_DATA_LENGTH_INFORMATION *info = ptr;

            if ((io->u.Status = server_get_unix_fd( handle, FILE_WRITE_DATA, &fd, &needs_close, NULL, NULL )))
                return io->u.Status;

            if (fstat( fd, &st ) == -1) io->u.Status = errno_to_status( errno );
            else if (info->ValidDataLength.QuadPart <= 0 || (off_t)info->ValidDataLength.QuadPart > st.st_size)
                io->u.Status = STATUS_INVALID_PARAMETER;
            else
            {
#ifdef HAVE_FALLOCATE
                if (fallocate( fd, 0, 0, (off_t)info->ValidDataLength.QuadPart ) == -1)
                {
                    NTSTATUS status = errno_to_status( errno );
                    if (status == STATUS_NOT_SUPPORTED) WARN( "fallocate not supported on this filesystem\n" );
                    else io->u.Status = status;
                }
#else
                FIXME( "setting valid data length not supported\n" );
#endif
            }
            if (needs_close) close( fd );
        }
        else io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileDispositionInformation:
        if (len >= sizeof(FILE_DISPOSITION_INFORMATION))
        {
            FILE_DISPOSITION_INFORMATION *info = ptr;

            SERVER_START_REQ( set_fd_disp_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->unlink   = info->DoDeleteFile;
                io->u.Status  = wine_server_call( req );
            }
            SERVER_END_REQ;
        } else
            io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileRenameInformation:
        if (len >= sizeof(FILE_RENAME_INFORMATION))
        {
            FILE_RENAME_INFORMATION *info = ptr;
            UNICODE_STRING name_str;
            OBJECT_ATTRIBUTES attr;
            char *unix_name;

            name_str.Buffer = info->FileName;
            name_str.Length = info->FileNameLength;
            name_str.MaximumLength = info->FileNameLength + sizeof(WCHAR);

            attr.Length = sizeof(attr);
            attr.ObjectName = &name_str;
            attr.RootDirectory = info->RootDirectory;
            attr.Attributes = OBJ_CASE_INSENSITIVE;

            io->u.Status = nt_to_unix_file_name_attr( &attr, &unix_name, FILE_OPEN_IF );
            if (io->u.Status != STATUS_SUCCESS && io->u.Status != STATUS_NO_SUCH_FILE)
                break;

            SERVER_START_REQ( set_fd_name_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->rootdir  = wine_server_obj_handle( attr.RootDirectory );
                req->link     = FALSE;
                req->replace  = info->ReplaceIfExists;
                wine_server_add_data( req, unix_name, strlen(unix_name) );
                io->u.Status = wine_server_call( req );
            }
            SERVER_END_REQ;

            free( unix_name );
        }
        else io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileLinkInformation:
        if (len >= sizeof(FILE_LINK_INFORMATION))
        {
            FILE_LINK_INFORMATION *info = ptr;
            UNICODE_STRING name_str;
            OBJECT_ATTRIBUTES attr;
            char *unix_name;

            name_str.Buffer = info->FileName;
            name_str.Length = info->FileNameLength;
            name_str.MaximumLength = info->FileNameLength + sizeof(WCHAR);

            attr.Length = sizeof(attr);
            attr.ObjectName = &name_str;
            attr.RootDirectory = info->RootDirectory;
            attr.Attributes = OBJ_CASE_INSENSITIVE;

            io->u.Status = nt_to_unix_file_name_attr( &attr, &unix_name, FILE_OPEN_IF );
            if (io->u.Status != STATUS_SUCCESS && io->u.Status != STATUS_NO_SUCH_FILE)
                break;

            SERVER_START_REQ( set_fd_name_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->rootdir  = wine_server_obj_handle( attr.RootDirectory );
                req->link     = TRUE;
                req->replace  = info->ReplaceIfExists;
                wine_server_add_data( req, unix_name, strlen(unix_name) );
                io->u.Status  = wine_server_call( req );
            }
            SERVER_END_REQ;

            free( unix_name );
        }
        else io->u.Status = STATUS_INVALID_PARAMETER_3;
        break;

    default:
        FIXME("Unsupported class (%d)\n", class);
        io->u.Status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    io->Information = 0;
    return io->u.Status;
}


/***********************************************************************
 *                  Asynchronous file I/O                              *
 */

typedef NTSTATUS async_callback_t( void *user, IO_STATUS_BLOCK *io, NTSTATUS status );

struct async_fileio
{
    async_callback_t    *callback; /* must be the first field */
    struct async_fileio *next;
    HANDLE               handle;
};

struct async_fileio_read
{
    struct async_fileio io;
    char               *buffer;
    unsigned int        already;
    unsigned int        count;
    BOOL                avail_mode;
};

struct async_fileio_write
{
    struct async_fileio io;
    const char         *buffer;
    unsigned int        already;
    unsigned int        count;
};

struct async_fileio_read_changes
{
    struct async_fileio io;
    void               *buffer;
    ULONG               buffer_size;
    ULONG               data_size;
    char                data[1];
};

struct async_irp
{
    struct async_fileio io;
    void               *buffer;   /* buffer for output */
    ULONG               size;     /* size of buffer */
};

static struct async_fileio *fileio_freelist;

static void release_fileio( struct async_fileio *io )
{
    for (;;)
    {
        struct async_fileio *next = fileio_freelist;
        io->next = next;
        if (InterlockedCompareExchangePointer( (void **)&fileio_freelist, io, next ) == next) return;
    }
}

static struct async_fileio *alloc_fileio( DWORD size, async_callback_t callback, HANDLE handle )
{
    /* first free remaining previous fileinfos */
    struct async_fileio *io = InterlockedExchangePointer( (void **)&fileio_freelist, NULL );

    while (io)
    {
        struct async_fileio *next = io->next;
        free( io );
        io = next;
    }

    if ((io = malloc( size )))
    {
        io->callback = callback;
        io->handle   = handle;
    }
    return io;
}

static async_data_t server_async( HANDLE handle, struct async_fileio *user, HANDLE event,
                                  PIO_APC_ROUTINE apc, void *apc_context, IO_STATUS_BLOCK *io )
{
    async_data_t async;
    async.handle      = wine_server_obj_handle( handle );
    async.user        = wine_server_client_ptr( user );
    async.iosb        = wine_server_client_ptr( io );
    async.event       = wine_server_obj_handle( event );
    async.apc         = wine_server_client_ptr( apc );
    async.apc_context = wine_server_client_ptr( apc_context );
    return async;
}

static NTSTATUS wait_async( HANDLE handle, BOOL alertable, IO_STATUS_BLOCK *io )
{
    if (NtWaitForSingleObject( handle, alertable, NULL )) return STATUS_PENDING;
    return io->u.Status;
}

/* callback for irp async I/O completion */
static NTSTATUS irp_completion( void *user, IO_STATUS_BLOCK *io, NTSTATUS status )
{
    struct async_irp *async = user;
    ULONG information = 0;

    if (status == STATUS_ALERTED)
    {
        SERVER_START_REQ( get_async_result )
        {
            req->user_arg = wine_server_client_ptr( async );
            wine_server_set_reply( req, async->buffer, async->size );
            status = virtual_locked_server_call( req );
            information = reply->size;
        }
        SERVER_END_REQ;
    }
    if (status != STATUS_PENDING)
    {
        io->u.Status = status;
        io->Information = information;
        release_fileio( &async->io );
    }
    return status;
}

static NTSTATUS async_read_proc( void *user, IO_STATUS_BLOCK *iosb, NTSTATUS status )
{
    struct async_fileio_read *fileio = user;
    int fd, needs_close, result;

    switch (status)
    {
    case STATUS_ALERTED: /* got some new data */
        /* check to see if the data is ready (non-blocking) */
        if ((status = server_get_unix_fd( fileio->io.handle, FILE_READ_DATA, &fd,
                                          &needs_close, NULL, NULL )))
            break;

        result = virtual_locked_read(fd, &fileio->buffer[fileio->already], fileio->count-fileio->already);
        if (needs_close) close( fd );

        if (result < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                status = STATUS_PENDING;
            else /* check to see if the transfer is complete */
                status = errno_to_status( errno );
        }
        else if (result == 0)
        {
            status = fileio->already ? STATUS_SUCCESS : STATUS_PIPE_BROKEN;
        }
        else
        {
            fileio->already += result;
            if (fileio->already >= fileio->count || fileio->avail_mode)
                status = STATUS_SUCCESS;
            else
                status = STATUS_PENDING;
        }
        break;

    case STATUS_TIMEOUT:
    case STATUS_IO_TIMEOUT:
        if (fileio->already) status = STATUS_SUCCESS;
        break;
    }
    if (status != STATUS_PENDING)
    {
        iosb->u.Status = status;
        iosb->Information = fileio->already;
        release_fileio( &fileio->io );
    }
    return status;
}

static NTSTATUS async_write_proc( void *user, IO_STATUS_BLOCK *iosb, NTSTATUS status )
{
    struct async_fileio_write *fileio = user;
    int result, fd, needs_close;
    enum server_fd_type type;

    switch (status)
    {
    case STATUS_ALERTED:
        /* write some data (non-blocking) */
        if ((status = server_get_unix_fd( fileio->io.handle, FILE_WRITE_DATA, &fd,
                                          &needs_close, &type, NULL )))
            break;

        if (!fileio->count && (type == FD_TYPE_MAILSLOT || type == FD_TYPE_SOCKET))
            result = send( fd, fileio->buffer, 0, 0 );
        else
            result = write( fd, &fileio->buffer[fileio->already], fileio->count - fileio->already );

        if (needs_close) close( fd );

        if (result < 0)
        {
            if (errno == EAGAIN || errno == EINTR) status = STATUS_PENDING;
            else status = errno_to_status( errno );
        }
        else
        {
            fileio->already += result;
            status = (fileio->already < fileio->count) ? STATUS_PENDING : STATUS_SUCCESS;
        }
        break;

    case STATUS_TIMEOUT:
    case STATUS_IO_TIMEOUT:
        if (fileio->already) status = STATUS_SUCCESS;
        break;
    }
    if (status != STATUS_PENDING)
    {
        iosb->u.Status = status;
        iosb->Information = fileio->already;
        release_fileio( &fileio->io );
    }
    return status;
}

/* do a read call through the server */
static NTSTATUS server_read_file( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                  IO_STATUS_BLOCK *io, void *buffer, ULONG size,
                                  LARGE_INTEGER *offset, ULONG *key )
{
    struct async_irp *async;
    NTSTATUS status;
    HANDLE wait_handle;
    ULONG options;

    if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
        return STATUS_NO_MEMORY;

    async->buffer  = buffer;
    async->size    = size;

    SERVER_START_REQ( read )
    {
        req->async = server_async( handle, &async->io, event, apc, apc_context, io );
        req->pos   = offset ? offset->QuadPart : 0;
        wine_server_set_reply( req, buffer, size );
        status = virtual_locked_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if (wait_handle && status != STATUS_PENDING)
        {
            io->u.Status    = status;
            io->Information = wine_server_reply_size( reply );
        }
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) free( async );

    if (wait_handle) status = wait_async( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT), io );
    return status;
}

/* do a write call through the server */
static NTSTATUS server_write_file( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                   IO_STATUS_BLOCK *io, const void *buffer, ULONG size,
                                   LARGE_INTEGER *offset, ULONG *key )
{
    struct async_irp *async;
    NTSTATUS status;
    HANDLE wait_handle;
    ULONG options;

    if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
        return STATUS_NO_MEMORY;

    async->buffer  = NULL;
    async->size    = 0;

    SERVER_START_REQ( write )
    {
        req->async = server_async( handle, &async->io, event, apc, apc_context, io );
        req->pos   = offset ? offset->QuadPart : 0;
        wine_server_add_data( req, buffer, size );
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if (wait_handle && status != STATUS_PENDING)
        {
            io->u.Status    = status;
            io->Information = reply->size;
        }
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) free( async );

    if (wait_handle) status = wait_async( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT), io );
    return status;
}

/* do an ioctl call through the server */
static NTSTATUS server_ioctl_file( HANDLE handle, HANDLE event,
                                   PIO_APC_ROUTINE apc, PVOID apc_context,
                                   IO_STATUS_BLOCK *io, ULONG code,
                                   const void *in_buffer, ULONG in_size,
                                   PVOID out_buffer, ULONG out_size )
{
    struct async_irp *async;
    NTSTATUS status;
    HANDLE wait_handle;
    ULONG options;

    if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
        return STATUS_NO_MEMORY;
    async->buffer  = out_buffer;
    async->size    = out_size;

    SERVER_START_REQ( ioctl )
    {
        req->code        = code;
        req->async       = server_async( handle, &async->io, event, apc, apc_context, io );
        wine_server_add_data( req, in_buffer, in_size );
        if ((code & 3) != METHOD_BUFFERED) wine_server_add_data( req, out_buffer, out_size );
        wine_server_set_reply( req, out_buffer, out_size );
        status = virtual_locked_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if (wait_handle && status != STATUS_PENDING)
        {
            io->u.Status    = status;
            io->Information = wine_server_reply_size( reply );
        }
    }
    SERVER_END_REQ;

    if (status == STATUS_NOT_SUPPORTED)
        WARN("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
             code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);

    if (status != STATUS_PENDING) free( async );

    if (wait_handle) status = wait_async( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT), io );
    return status;
}


struct io_timeouts
{
    int interval;   /* max interval between two bytes */
    int total;      /* total timeout for the whole operation */
    int end_time;   /* absolute time of end of operation */
};

/* retrieve the I/O timeouts to use for a given handle */
static NTSTATUS get_io_timeouts( HANDLE handle, enum server_fd_type type, ULONG count, BOOL is_read,
                                 struct io_timeouts *timeouts )
{
    NTSTATUS status = STATUS_SUCCESS;

    timeouts->interval = timeouts->total = -1;

    switch(type)
    {
    case FD_TYPE_SERIAL:
    {
        /* GetCommTimeouts */
        SERIAL_TIMEOUTS st;
        IO_STATUS_BLOCK io;

        status = NtDeviceIoControlFile( handle, NULL, NULL, NULL, &io,
                                        IOCTL_SERIAL_GET_TIMEOUTS, NULL, 0, &st, sizeof(st) );
        if (status) break;

        if (is_read)
        {
            if (st.ReadIntervalTimeout)
                timeouts->interval = st.ReadIntervalTimeout;

            if (st.ReadTotalTimeoutMultiplier || st.ReadTotalTimeoutConstant)
            {
                timeouts->total = st.ReadTotalTimeoutConstant;
                if (st.ReadTotalTimeoutMultiplier != MAXDWORD)
                    timeouts->total += count * st.ReadTotalTimeoutMultiplier;
            }
            else if (st.ReadIntervalTimeout == MAXDWORD)
                timeouts->interval = timeouts->total = 0;
        }
        else  /* write */
        {
            if (st.WriteTotalTimeoutMultiplier || st.WriteTotalTimeoutConstant)
            {
                timeouts->total = st.WriteTotalTimeoutConstant;
                if (st.WriteTotalTimeoutMultiplier != MAXDWORD)
                    timeouts->total += count * st.WriteTotalTimeoutMultiplier;
            }
        }
        break;
    }
    case FD_TYPE_MAILSLOT:
        if (is_read)
        {
            timeouts->interval = 0;  /* return as soon as we got something */
            SERVER_START_REQ( set_mailslot_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags = 0;
                if (!(status = wine_server_call( req )) &&
                    reply->read_timeout != TIMEOUT_INFINITE)
                    timeouts->total = reply->read_timeout / -10000;
            }
            SERVER_END_REQ;
        }
        break;
    case FD_TYPE_SOCKET:
    case FD_TYPE_CHAR:
        if (is_read) timeouts->interval = 0;  /* return as soon as we got something */
        break;
    default:
        break;
    }
    if (timeouts->total != -1) timeouts->end_time = NtGetTickCount() + timeouts->total;
    return STATUS_SUCCESS;
}


/* retrieve the timeout for the next wait, in milliseconds */
static inline int get_next_io_timeout( const struct io_timeouts *timeouts, ULONG already )
{
    int ret = -1;

    if (timeouts->total != -1)
    {
        ret = timeouts->end_time - NtGetTickCount();
        if (ret < 0) ret = 0;
    }
    if (already && timeouts->interval != -1)
    {
        if (ret == -1 || ret > timeouts->interval) ret = timeouts->interval;
    }
    return ret;
}


/* retrieve the avail_mode flag for async reads */
static NTSTATUS get_io_avail_mode( HANDLE handle, enum server_fd_type type, BOOL *avail_mode )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch(type)
    {
    case FD_TYPE_SERIAL:
    {
        /* GetCommTimeouts */
        SERIAL_TIMEOUTS st;
        IO_STATUS_BLOCK io;

        status = NtDeviceIoControlFile( handle, NULL, NULL, NULL, &io,
                                        IOCTL_SERIAL_GET_TIMEOUTS, NULL, 0, &st, sizeof(st) );
        if (status) break;
        *avail_mode = (!st.ReadTotalTimeoutMultiplier &&
                       !st.ReadTotalTimeoutConstant &&
                       st.ReadIntervalTimeout == MAXDWORD);
        break;
    }
    case FD_TYPE_MAILSLOT:
    case FD_TYPE_SOCKET:
    case FD_TYPE_CHAR:
        *avail_mode = TRUE;
        break;
    default:
        *avail_mode = FALSE;
        break;
    }
    return status;
}

/* register an async I/O for a file read; helper for NtReadFile */
static NTSTATUS register_async_file_read( HANDLE handle, HANDLE event,
                                          PIO_APC_ROUTINE apc, void *apc_user,
                                          IO_STATUS_BLOCK *iosb, void *buffer,
                                          ULONG already, ULONG length, BOOL avail_mode )
{
    struct async_fileio_read *fileio;
    NTSTATUS status;

    if (!(fileio = (struct async_fileio_read *)alloc_fileio( sizeof(*fileio), async_read_proc, handle )))
        return STATUS_NO_MEMORY;

    fileio->already = already;
    fileio->count = length;
    fileio->buffer = buffer;
    fileio->avail_mode = avail_mode;

    SERVER_START_REQ( register_async )
    {
        req->type   = ASYNC_TYPE_READ;
        req->count  = length;
        req->async  = server_async( handle, &fileio->io, event, apc, apc_user, iosb );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) free( fileio );
    return status;
}

static void add_completion( HANDLE handle, ULONG_PTR value, NTSTATUS status, ULONG info, BOOL async )
{
    SERVER_START_REQ( add_fd_completion )
    {
        req->handle      = wine_server_obj_handle( handle );
        req->cvalue      = value;
        req->status      = status;
        req->information = info;
        req->async       = async;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

static NTSTATUS set_pending_write( HANDLE device )
{
    NTSTATUS status;

    SERVER_START_REQ( set_serial_info )
    {
        req->handle = wine_server_obj_handle( device );
        req->flags  = SERIALINFO_PENDING_WRITE;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}


/******************************************************************************
 *              NtReadFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtReadFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                            IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                            LARGE_INTEGER *offset, ULONG *key )
{
    int result, unix_handle, needs_close;
    unsigned int options;
    struct io_timeouts timeouts;
    NTSTATUS status, ret_status;
    ULONG total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE, async_read, timeout_init_done = FALSE;

    TRACE( "(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p)\n",
           handle, event, apc, apc_user, io, buffer, length, offset, key );

    if (!io) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( handle, FILE_READ_DATA, &unix_handle, &needs_close, &type, &options );
    if (status && status != STATUS_BAD_DEVICE_TYPE) return status;

    if (!virtual_check_buffer_for_write( buffer, length )) return STATUS_ACCESS_VIOLATION;

    if (status == STATUS_BAD_DEVICE_TYPE)
        return server_read_file( handle, event, apc, apc_user, io, buffer, length, offset, key );

    async_read = !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));

    if (type == FD_TYPE_FILE)
    {
        if (async_read && (!offset || offset->QuadPart < 0))
        {
            status = STATUS_INVALID_PARAMETER;
            goto done;
        }

        if (offset && offset->QuadPart != FILE_USE_FILE_POINTER_POSITION)
        {
            /* async I/O doesn't make sense on regular files */
            while ((result = virtual_locked_pread( unix_handle, buffer, length, offset->QuadPart )) == -1)
            {
                if (errno != EINTR)
                {
                    status = errno_to_status( errno );
                    goto done;
                }
            }
            if (!async_read) /* update file pointer position */
                lseek( unix_handle, offset->QuadPart + result, SEEK_SET );

            total = result;
            status = (total || !length) ? STATUS_SUCCESS : STATUS_END_OF_FILE;
            goto done;
        }
    }
    else if (type == FD_TYPE_SERIAL || type == FD_TYPE_DEVICE)
    {
        if (async_read && (!offset || offset->QuadPart < 0))
        {
            status = STATUS_INVALID_PARAMETER;
            goto done;
        }
    }

    if (type == FD_TYPE_SERIAL && async_read && length)
    {
        /* an asynchronous serial port read with a read interval timeout needs to
           skip the synchronous read to make sure that the server starts the read
           interval timer after the first read */
        if ((status = get_io_timeouts( handle, type, length, TRUE, &timeouts ))) goto err;
        if (timeouts.interval)
        {
            status = register_async_file_read( handle, event, apc, apc_user, io,
                                               buffer, total, length, FALSE );
            goto err;
        }
    }

    for (;;)
    {
        if ((result = virtual_locked_read( unix_handle, (char *)buffer + total, length - total )) >= 0)
        {
            total += result;
            if (!result || total == length)
            {
                if (total)
                {
                    status = STATUS_SUCCESS;
                    goto done;
                }
                switch (type)
                {
                case FD_TYPE_FILE:
                case FD_TYPE_CHAR:
                case FD_TYPE_DEVICE:
                    status = length ? STATUS_END_OF_FILE : STATUS_SUCCESS;
                    goto done;
                case FD_TYPE_SERIAL:
                    if (!length)
                    {
                        status = STATUS_SUCCESS;
                        goto done;
                    }
                    break;
                default:
                    status = STATUS_PIPE_BROKEN;
                    goto err;
                }
            }
            else if (type == FD_TYPE_FILE) continue;  /* no async I/O on regular files */
        }
        else if (errno != EAGAIN)
        {
            if (errno == EINTR) continue;
            if (!total) status = errno_to_status( errno );
            goto err;
        }

        if (async_read)
        {
            BOOL avail_mode;

            if ((status = get_io_avail_mode( handle, type, &avail_mode ))) goto err;
            if (total && avail_mode)
            {
                status = STATUS_SUCCESS;
                goto done;
            }
            status = register_async_file_read( handle, event, apc, apc_user, io,
                                               buffer, total, length, avail_mode );
            goto err;
        }
        else  /* synchronous read, wait for the fd to become ready */
        {
            struct pollfd pfd;
            int ret, timeout;

            if (!timeout_init_done)
            {
                timeout_init_done = TRUE;
                if ((status = get_io_timeouts( handle, type, length, TRUE, &timeouts ))) goto err;
                if (event) NtResetEvent( event, NULL );
            }
            timeout = get_next_io_timeout( &timeouts, total );

            pfd.fd = unix_handle;
            pfd.events = POLLIN;

            if (!timeout || !(ret = poll( &pfd, 1, timeout )))
            {
                if (total)  /* return with what we got so far */
                    status = STATUS_SUCCESS;
                else
                    status = (type == FD_TYPE_MAILSLOT) ? STATUS_IO_TIMEOUT : STATUS_TIMEOUT;
                goto done;
            }
            if (ret == -1 && errno != EINTR)
            {
                status = errno_to_status( errno );
                goto done;
            }
            /* will now restart the read */
        }
    }

done:
    send_completion = cvalue != 0;

err:
    if (needs_close) close( unix_handle );
    if (status == STATUS_SUCCESS || (status == STATUS_END_OF_FILE && (!async_read || type == FD_TYPE_FILE)))
    {
        io->u.Status = status;
        io->Information = total;
        TRACE("= SUCCESS (%u)\n", total);
        if (event) NtSetEvent( event, NULL );
        if (apc && (!status || async_read)) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                              (ULONG_PTR)apc_user, (ULONG_PTR)io, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }

    ret_status = async_read && type == FD_TYPE_FILE && (status == STATUS_SUCCESS || status == STATUS_END_OF_FILE)
            ? STATUS_PENDING : status;

    if (send_completion) add_completion( handle, cvalue, status, total, ret_status == STATUS_PENDING );
    return ret_status;
}


/******************************************************************************
 *              NtReadFileScatter   (NTDLL.@)
 */
NTSTATUS WINAPI NtReadFileScatter( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                   IO_STATUS_BLOCK *io, FILE_SEGMENT_ELEMENT *segments,
                                   ULONG length, LARGE_INTEGER *offset, ULONG *key )
{
    int result, unix_handle, needs_close;
    unsigned int options;
    NTSTATUS status;
    ULONG pos = 0, total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE;

    TRACE( "(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p),partial stub!\n",
           file, event, apc, apc_user, io, segments, length, offset, key );

    if (!io) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( file, FILE_READ_DATA, &unix_handle, &needs_close, &type, &options );
    if (status) return status;

    if ((type != FD_TYPE_FILE) ||
        (options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) ||
        !(options & FILE_NO_INTERMEDIATE_BUFFERING))
    {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    while (length)
    {
        if (offset && offset->QuadPart != FILE_USE_FILE_POINTER_POSITION)
            result = pread( unix_handle, (char *)segments->Buffer + pos,
                            min( length - pos, page_size - pos ), offset->QuadPart + total );
        else
            result = read( unix_handle, (char *)segments->Buffer + pos, min( length - pos, page_size - pos ) );

        if (result == -1)
        {
            if (errno == EINTR) continue;
            status = errno_to_status( errno );
            break;
        }
        if (!result) break;
        total += result;
        length -= result;
        if ((pos += result) == page_size)
        {
            pos = 0;
            segments++;
        }
    }

    if (total == 0) status = STATUS_END_OF_FILE;

    send_completion = cvalue != 0;

    if (needs_close) close( unix_handle );
    io->u.Status = status;
    io->Information = total;
    TRACE("= 0x%08x (%u)\n", status, total);
    if (event) NtSetEvent( event, NULL );
    if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                               (ULONG_PTR)apc_user, (ULONG_PTR)io, 0 );
    if (send_completion) add_completion( file, cvalue, status, total, TRUE );

    return STATUS_PENDING;

error:
    if (needs_close) close( unix_handle );
    if (event) NtResetEvent( event, NULL );
    TRACE("= 0x%08x\n", status);
    return status;
}


/******************************************************************************
 *              NtWriteFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtWriteFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                             IO_STATUS_BLOCK *io, const void *buffer, ULONG length,
                             LARGE_INTEGER *offset, ULONG *key )
{
    int result, unix_handle, needs_close;
    unsigned int options;
    struct io_timeouts timeouts;
    NTSTATUS status, ret_status;
    ULONG total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE, async_write, append_write = FALSE, timeout_init_done = FALSE;
    LARGE_INTEGER offset_eof;

    TRACE( "(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p)\n",
           handle, event, apc, apc_user, io, buffer, length, offset, key );

    if (!io) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( handle, FILE_WRITE_DATA, &unix_handle, &needs_close, &type, &options );
    if (status == STATUS_ACCESS_DENIED)
    {
        status = server_get_unix_fd( handle, FILE_APPEND_DATA, &unix_handle,
                                     &needs_close, &type, &options );
        append_write = TRUE;
    }
    if (status && status != STATUS_BAD_DEVICE_TYPE) return status;

    async_write = !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));

    if (!virtual_check_buffer_for_read( buffer, length ))
    {
        status = STATUS_INVALID_USER_BUFFER;
        goto done;
    }

    if (status == STATUS_BAD_DEVICE_TYPE)
        return server_write_file( handle, event, apc, apc_user, io, buffer, length, offset, key );

    if (type == FD_TYPE_FILE)
    {
        if (async_write &&
            (!offset || (offset->QuadPart < 0 && offset->QuadPart != FILE_WRITE_TO_END_OF_FILE)))
        {
            status = STATUS_INVALID_PARAMETER;
            goto done;
        }

        if (append_write)
        {
            offset_eof.QuadPart = FILE_WRITE_TO_END_OF_FILE;
            offset = &offset_eof;
        }

        if (offset && offset->QuadPart != FILE_USE_FILE_POINTER_POSITION)
        {
            off_t off = offset->QuadPart;

            if (offset->QuadPart == FILE_WRITE_TO_END_OF_FILE)
            {
                struct stat st;

                if (fstat( unix_handle, &st ) == -1)
                {
                    status = errno_to_status( errno );
                    goto done;
                }
                off = st.st_size;
            }
            else if (offset->QuadPart < 0)
            {
                status = STATUS_INVALID_PARAMETER;
                goto done;
            }

            /* async I/O doesn't make sense on regular files */
            while ((result = pwrite( unix_handle, buffer, length, off )) == -1)
            {
                if (errno != EINTR)
                {
                    if (errno == EFAULT) status = STATUS_INVALID_USER_BUFFER;
                    else status = errno_to_status( errno );
                    goto done;
                }
            }

            if (!async_write) /* update file pointer position */
                lseek( unix_handle, off + result, SEEK_SET );

            total = result;
            status = STATUS_SUCCESS;
            goto done;
        }
    }
    else if (type == FD_TYPE_SERIAL || type == FD_TYPE_DEVICE)
    {
        if (async_write &&
            (!offset || (offset->QuadPart < 0 && offset->QuadPart != FILE_WRITE_TO_END_OF_FILE)))
        {
            status = STATUS_INVALID_PARAMETER;
            goto done;
        }
    }

    for (;;)
    {
        /* zero-length writes on sockets may not work with plain write(2) */
        if (!length && (type == FD_TYPE_MAILSLOT || type == FD_TYPE_SOCKET))
            result = send( unix_handle, buffer, 0, 0 );
        else
            result = write( unix_handle, (const char *)buffer + total, length - total );

        if (result >= 0)
        {
            total += result;
            if (total == length)
            {
                status = STATUS_SUCCESS;
                goto done;
            }
            if (type == FD_TYPE_FILE) continue;  /* no async I/O on regular files */
        }
        else if (errno != EAGAIN)
        {
            if (errno == EINTR) continue;
            if (!total)
            {
                if (errno == EFAULT) status = STATUS_INVALID_USER_BUFFER;
                else status = errno_to_status( errno );
            }
            goto err;
        }

        if (async_write)
        {
            struct async_fileio_write *fileio;

            fileio = (struct async_fileio_write *)alloc_fileio( sizeof(*fileio), async_write_proc, handle );
            if (!fileio)
            {
                status = STATUS_NO_MEMORY;
                goto err;
            }
            fileio->already = total;
            fileio->count = length;
            fileio->buffer = buffer;

            SERVER_START_REQ( register_async )
            {
                req->type   = ASYNC_TYPE_WRITE;
                req->count  = length;
                req->async  = server_async( handle, &fileio->io, event, apc, apc_user, io );
                status = wine_server_call( req );
            }
            SERVER_END_REQ;

            if (status != STATUS_PENDING) free( fileio );
            goto err;
        }
        else  /* synchronous write, wait for the fd to become ready */
        {
            struct pollfd pfd;
            int ret, timeout;

            if (!timeout_init_done)
            {
                timeout_init_done = TRUE;
                if ((status = get_io_timeouts( handle, type, length, FALSE, &timeouts )))
                    goto err;
                if (event) NtResetEvent( event, NULL );
            }
            timeout = get_next_io_timeout( &timeouts, total );

            pfd.fd = unix_handle;
            pfd.events = POLLOUT;

            if (!timeout || !(ret = poll( &pfd, 1, timeout )))
            {
                /* return with what we got so far */
                status = total ? STATUS_SUCCESS : STATUS_TIMEOUT;
                goto done;
            }
            if (ret == -1 && errno != EINTR)
            {
                status = errno_to_status( errno );
                goto done;
            }
            /* will now restart the write */
        }
    }

done:
    send_completion = cvalue != 0;

err:
    if (needs_close) close( unix_handle );

    if (type == FD_TYPE_SERIAL && (status == STATUS_SUCCESS || status == STATUS_PENDING))
        set_pending_write( handle );

    if (status == STATUS_SUCCESS)
    {
        io->u.Status = status;
        io->Information = total;
        TRACE("= SUCCESS (%u)\n", total);
        if (event) NtSetEvent( event, NULL );
        if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                   (ULONG_PTR)apc_user, (ULONG_PTR)io, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }

    ret_status = async_write && type == FD_TYPE_FILE && status == STATUS_SUCCESS ? STATUS_PENDING : status;
    if (send_completion) add_completion( handle, cvalue, status, total, ret_status == STATUS_PENDING );
    return ret_status;
}


/******************************************************************************
 *              NtWriteFileGather   (NTDLL.@)
 */
NTSTATUS WINAPI NtWriteFileGather( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                   IO_STATUS_BLOCK *io, FILE_SEGMENT_ELEMENT *segments,
                                   ULONG length, LARGE_INTEGER *offset, ULONG *key )
{
    int result, unix_handle, needs_close;
    unsigned int options;
    NTSTATUS status;
    ULONG pos = 0, total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE;

    TRACE( "(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p),partial stub!\n",
           file, event, apc, apc_user, io, segments, length, offset, key );

    if (length % page_size) return STATUS_INVALID_PARAMETER;
    if (!io) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( file, FILE_WRITE_DATA, &unix_handle, &needs_close, &type, &options );
    if (status) return status;

    if ((type != FD_TYPE_FILE) ||
        (options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) ||
        !(options & FILE_NO_INTERMEDIATE_BUFFERING))
    {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    while (length)
    {
        if (offset && offset->QuadPart != FILE_USE_FILE_POINTER_POSITION)
            result = pwrite( unix_handle, (char *)segments->Buffer + pos,
                             page_size - pos, offset->QuadPart + total );
        else
            result = write( unix_handle, (char *)segments->Buffer + pos, page_size - pos );

        if (result == -1)
        {
            if (errno == EINTR) continue;
            if (errno == EFAULT)
            {
                status = STATUS_INVALID_USER_BUFFER;
                goto done;
            }
            status = errno_to_status( errno );
            break;
        }
        if (!result)
        {
            status = STATUS_DISK_FULL;
            break;
        }
        total += result;
        length -= result;
        if ((pos += result) == page_size)
        {
            pos = 0;
            segments++;
        }
    }

    send_completion = cvalue != 0;

 done:
    if (needs_close) close( unix_handle );
    if (status == STATUS_SUCCESS)
    {
        io->u.Status = status;
        io->Information = total;
        TRACE("= SUCCESS (%u)\n", total);
        if (event) NtSetEvent( event, NULL );
        if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                   (ULONG_PTR)apc_user, (ULONG_PTR)io, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }
    if (send_completion) add_completion( file, cvalue, status, total, FALSE );
    return status;
}


/******************************************************************************
 *              NtDeviceIoControlFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtDeviceIoControlFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                       IO_STATUS_BLOCK *io, ULONG code, void *in_buffer, ULONG in_size,
                                       void *out_buffer, ULONG out_size )
{
    ULONG device = (code >> 16);
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    TRACE( "(%p,%p,%p,%p,%p,0x%08x,%p,0x%08x,%p,0x%08x)\n",
           handle, event, apc, apc_context, io, code, in_buffer, in_size, out_buffer, out_size );

    switch (device)
    {
    case FILE_DEVICE_DISK:
    case FILE_DEVICE_CD_ROM:
    case FILE_DEVICE_DVD:
    case FILE_DEVICE_CONTROLLER:
    case FILE_DEVICE_MASS_STORAGE:
        status = cdrom_DeviceIoControl( handle, event, apc, apc_context, io, code,
                                        in_buffer, in_size, out_buffer, out_size );
        break;
    case FILE_DEVICE_SERIAL_PORT:
        status = serial_DeviceIoControl( handle, event, apc, apc_context, io, code,
                                         in_buffer, in_size, out_buffer, out_size );
        break;
    case FILE_DEVICE_TAPE:
        status = tape_DeviceIoControl( handle, event, apc, apc_context, io, code,
                                       in_buffer, in_size, out_buffer, out_size );
        break;
    }

    if (status == STATUS_NOT_SUPPORTED || status == STATUS_BAD_DEVICE_TYPE)
        return server_ioctl_file( handle, event, apc, apc_context, io, code,
                                  in_buffer, in_size, out_buffer, out_size );

    if (status != STATUS_PENDING) io->u.Status = status;
    return status;
}


/* Tell Valgrind to ignore any holes in structs we will be passing to the
 * server */
static void ignore_server_ioctl_struct_holes( ULONG code, const void *in_buffer, ULONG in_size )
{
#ifdef VALGRIND_MAKE_MEM_DEFINED
# define IGNORE_STRUCT_HOLE(buf, size, t, f1, f2) \
    do { \
        if (FIELD_OFFSET(t, f1) + sizeof(((t *)0)->f1) < FIELD_OFFSET(t, f2)) \
            if ((size) >= FIELD_OFFSET(t, f2)) \
                VALGRIND_MAKE_MEM_DEFINED( \
                    (const char *)(buf) + FIELD_OFFSET(t, f1) + sizeof(((t *)0)->f1), \
                    FIELD_OFFSET(t, f2) - FIELD_OFFSET(t, f1) + sizeof(((t *)0)->f1)); \
    } while (0)

    switch (code)
    {
    case FSCTL_PIPE_WAIT:
        IGNORE_STRUCT_HOLE(in_buffer, in_size, FILE_PIPE_WAIT_FOR_BUFFER, TimeoutSpecified, Name);
        break;
    }
#endif
}


/******************************************************************************
 *              NtFsControlFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtFsControlFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                 IO_STATUS_BLOCK *io, ULONG code, void *in_buffer, ULONG in_size,
                                 void *out_buffer, ULONG out_size )
{
    NTSTATUS status;

    TRACE( "(%p,%p,%p,%p,%p,0x%08x,%p,0x%08x,%p,0x%08x)\n",
           handle, event, apc, apc_context, io, code, in_buffer, in_size, out_buffer, out_size );

    if (!io) return STATUS_INVALID_PARAMETER;

    ignore_server_ioctl_struct_holes( code, in_buffer, in_size );

    switch (code)
    {
    case FSCTL_DISMOUNT_VOLUME:
        status = server_ioctl_file( handle, event, apc, apc_context, io, code,
                                    in_buffer, in_size, out_buffer, out_size );
        if (!status) status = unmount_device( handle );
        return status;

    case FSCTL_PIPE_IMPERSONATE:
        FIXME("FSCTL_PIPE_IMPERSONATE: impersonating self\n");
        return server_ioctl_file( handle, event, apc, apc_context, io, code,
                                  in_buffer, in_size, out_buffer, out_size );

    case FSCTL_IS_VOLUME_MOUNTED:
    case FSCTL_LOCK_VOLUME:
    case FSCTL_UNLOCK_VOLUME:
        FIXME("stub! return success - Unsupported fsctl %x (device=%x access=%x func=%x method=%x)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
        status = STATUS_SUCCESS;
        break;

    case FSCTL_GET_RETRIEVAL_POINTERS:
    {
        RETRIEVAL_POINTERS_BUFFER *buffer = (RETRIEVAL_POINTERS_BUFFER *)out_buffer;

        FIXME("stub: FSCTL_GET_RETRIEVAL_POINTERS\n");

        if (out_size >= sizeof(RETRIEVAL_POINTERS_BUFFER))
        {
            buffer->ExtentCount                 = 1;
            buffer->StartingVcn.QuadPart        = 1;
            buffer->Extents[0].NextVcn.QuadPart = 0;
            buffer->Extents[0].Lcn.QuadPart     = 0;
            io->Information = sizeof(RETRIEVAL_POINTERS_BUFFER);
            status = STATUS_SUCCESS;
        }
        else
        {
            io->Information = 0;
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }

    case FSCTL_GET_OBJECT_ID:
    {
        FILE_OBJECTID_BUFFER *info = out_buffer;
        int fd, needs_close;
        struct stat st;

        io->Information = 0;
        if (out_size >= sizeof(*info))
        {
            status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL );
            if (status) break;
            fstat( fd, &st );
            if (needs_close) close( fd );
            memset( info, 0, sizeof(*info) );
            memcpy( info->ObjectId, &st.st_dev, sizeof(st.st_dev) );
            memcpy( info->ObjectId + 8, &st.st_ino, sizeof(st.st_ino) );
            io->Information = sizeof(*info);
        }
        else status = STATUS_BUFFER_TOO_SMALL;
        break;
    }

    case FSCTL_SET_SPARSE:
        TRACE("FSCTL_SET_SPARSE: Ignoring request\n");
        io->Information = 0;
        status = STATUS_SUCCESS;
        break;
    default:
        return server_ioctl_file( handle, event, apc, apc_context, io, code,
                                  in_buffer, in_size, out_buffer, out_size );
    }

    if (status != STATUS_PENDING) io->u.Status = status;
    return status;
}


/******************************************************************************
 *              NtFlushBuffersFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushBuffersFile( HANDLE handle, IO_STATUS_BLOCK *io )
{
    NTSTATUS ret;
    HANDLE wait_handle;
    enum server_fd_type type;
    int fd, needs_close;

    if (!io || !virtual_check_buffer_for_write( io, sizeof(*io) )) return STATUS_ACCESS_VIOLATION;

    ret = server_get_unix_fd( handle, FILE_WRITE_DATA, &fd, &needs_close, &type, NULL );
    if (ret == STATUS_ACCESS_DENIED)
        ret = server_get_unix_fd( handle, FILE_APPEND_DATA, &fd, &needs_close, &type, NULL );

    if (!ret && (type == FD_TYPE_FILE || type == FD_TYPE_DIR || type == FD_TYPE_CHAR))
    {
        if (fsync(fd)) ret = errno_to_status( errno );
        io->u.Status    = ret;
        io->Information = 0;
    }
    else if (!ret && type == FD_TYPE_SERIAL)
    {
        ret = serial_FlushBuffersFile( fd );
    }
    else if (ret != STATUS_ACCESS_DENIED)
    {
        struct async_irp *async;

        if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
            return STATUS_NO_MEMORY;
        async->buffer  = NULL;
        async->size    = 0;

        SERVER_START_REQ( flush )
        {
            req->async = server_async( handle, &async->io, NULL, NULL, NULL, io );
            ret = wine_server_call( req );
            wait_handle = wine_server_ptr_handle( reply->event );
            if (wait_handle && ret != STATUS_PENDING)
            {
                io->u.Status    = ret;
                io->Information = 0;
            }
        }
        SERVER_END_REQ;

        if (ret != STATUS_PENDING) free( async );

        if (wait_handle) ret = wait_async( wait_handle, FALSE, io );
    }

    if (needs_close) close( fd );
    return ret;
}


/**************************************************************************
 *           NtCancelIoFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtCancelIoFile( HANDLE handle, IO_STATUS_BLOCK *io_status )
{
    TRACE( "%p %p\n", handle, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( handle );
        req->only_thread = TRUE;
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return io_status->u.Status;
}


/**************************************************************************
 *           NtCancelIoFileEx   (NTDLL.@)
 */
NTSTATUS WINAPI NtCancelIoFileEx( HANDLE handle, IO_STATUS_BLOCK *io, IO_STATUS_BLOCK *io_status )
{
    TRACE( "%p %p %p\n", handle, io, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle = wine_server_obj_handle( handle );
        req->iosb   = wine_server_client_ptr( io );
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return io_status->u.Status;
}


/******************************************************************
 *           NtLockFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtLockFile( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void* apc_user,
                            IO_STATUS_BLOCK *io_status, LARGE_INTEGER *offset,
                            LARGE_INTEGER *count, ULONG *key, BOOLEAN dont_wait, BOOLEAN exclusive )
{
    static int warn;
    NTSTATUS ret;
    HANDLE handle;
    BOOLEAN async;

    if (apc || io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    if (apc_user && !warn++) FIXME("I/O completion on lock not implemented yet\n");

    for (;;)
    {
        SERVER_START_REQ( lock_file )
        {
            req->handle      = wine_server_obj_handle( file );
            req->offset      = offset->QuadPart;
            req->count       = count->QuadPart;
            req->shared      = !exclusive;
            req->wait        = !dont_wait;
            ret = wine_server_call( req );
            handle = wine_server_ptr_handle( reply->handle );
            async  = reply->overlapped;
        }
        SERVER_END_REQ;
        if (ret != STATUS_PENDING)
        {
            if (!ret && event) NtSetEvent( event, NULL );
            return ret;
        }
        if (async)
        {
            FIXME( "Async I/O lock wait not implemented, might deadlock\n" );
            if (handle) NtClose( handle );
            return STATUS_PENDING;
        }
        if (handle)
        {
            NtWaitForSingleObject( handle, FALSE, NULL );
            NtClose( handle );
        }
        else  /* Unix lock conflict, sleep a bit and retry */
        {
            LARGE_INTEGER time;
            time.QuadPart = -100 * (ULONGLONG)10000;
            NtDelayExecution( FALSE, &time );
        }
    }
}


/******************************************************************
 *           NtUnlockFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtUnlockFile( HANDLE handle, IO_STATUS_BLOCK *io_status, LARGE_INTEGER *offset,
                              LARGE_INTEGER *count, ULONG *key )
{
    NTSTATUS status;

    TRACE( "%p %x%08x %x%08x\n",
           handle, offset->u.HighPart, offset->u.LowPart, count->u.HighPart, count->u.LowPart );

    if (io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    SERVER_START_REQ( unlock_file )
    {
        req->handle = wine_server_obj_handle( handle );
        req->offset = offset->QuadPart;
        req->count  = count->QuadPart;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}


static NTSTATUS read_changes_apc( void *user, IO_STATUS_BLOCK *iosb, NTSTATUS status )
{
    struct async_fileio_read_changes *fileio = user;
    int size = 0;

    if (status == STATUS_ALERTED)
    {
        SERVER_START_REQ( read_change )
        {
            req->handle = wine_server_obj_handle( fileio->io.handle );
            wine_server_set_reply( req, fileio->data, fileio->data_size );
            status = wine_server_call( req );
            size = wine_server_reply_size( reply );
        }
        SERVER_END_REQ;

        if (status == STATUS_SUCCESS && fileio->buffer)
        {
            FILE_NOTIFY_INFORMATION *pfni = fileio->buffer;
            int i, left = fileio->buffer_size;
            DWORD *last_entry_offset = NULL;
            struct filesystem_event *event = (struct filesystem_event*)fileio->data;

            while (size && left >= sizeof(*pfni))
            {
                DWORD len = (left - offsetof(FILE_NOTIFY_INFORMATION, FileName)) / sizeof(WCHAR);

                /* convert to an NT style path */
                for (i = 0; i < event->len; i++)
                    if (event->name[i] == '/') event->name[i] = '\\';

                pfni->Action = event->action;
                pfni->FileNameLength = ntdll_umbstowcs( event->name, event->len, pfni->FileName, len );
                last_entry_offset = &pfni->NextEntryOffset;

                if (pfni->FileNameLength == len) break;

                i = offsetof(FILE_NOTIFY_INFORMATION, FileName[pfni->FileNameLength]);
                pfni->FileNameLength *= sizeof(WCHAR);
                pfni->NextEntryOffset = i;
                pfni = (FILE_NOTIFY_INFORMATION*)((char*)pfni + i);
                left -= i;

                i = (offsetof(struct filesystem_event, name[event->len])
                     + sizeof(int)-1) / sizeof(int) * sizeof(int);
                event = (struct filesystem_event*)((char*)event + i);
                size -= i;
            }

            if (size)
            {
                status = STATUS_NOTIFY_ENUM_DIR;
                size = 0;
            }
            else
            {
                if (last_entry_offset) *last_entry_offset = 0;
                size = fileio->buffer_size - left;
            }
        }
        else
        {
            status = STATUS_NOTIFY_ENUM_DIR;
            size = 0;
        }
    }

    if (status != STATUS_PENDING)
    {
        iosb->u.Status = status;
        iosb->Information = size;
        release_fileio( &fileio->io );
    }
    return status;
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
 *              NtNotifyChangeDirectoryFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtNotifyChangeDirectoryFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                             void *apc_context, IO_STATUS_BLOCK *iosb, void *buffer,
                                             ULONG buffer_size, ULONG filter, BOOLEAN subtree )
{
    struct async_fileio_read_changes *fileio;
    NTSTATUS status;
    ULONG size = max( 4096, buffer_size );

    TRACE( "%p %p %p %p %p %p %u %u %d\n",
           handle, event, apc, apc_context, iosb, buffer, buffer_size, filter, subtree );

    if (!iosb) return STATUS_ACCESS_VIOLATION;
    if (filter == 0 || (filter & ~FILE_NOTIFY_ALL)) return STATUS_INVALID_PARAMETER;

    fileio = (struct async_fileio_read_changes *)alloc_fileio(
        offsetof(struct async_fileio_read_changes, data[size]), read_changes_apc, handle );
    if (!fileio) return STATUS_NO_MEMORY;

    fileio->buffer      = buffer;
    fileio->buffer_size = buffer_size;
    fileio->data_size   = size;

    SERVER_START_REQ( read_directory_changes )
    {
        req->filter    = filter;
        req->want_data = (buffer != NULL);
        req->subtree   = subtree;
        req->async     = server_async( handle, &fileio->io, event, apc, apc_context, iosb );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) free( fileio );
    return status;
}


#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
/* helper for FILE_GetDeviceInfo to hide some platform differences in fstatfs */
static inline void get_device_info_fstatfs( FILE_FS_DEVICE_INFORMATION *info, const char *fstypename,
                                            unsigned int flags )
{
    if (!strcmp("cd9660", fstypename) || !strcmp("udf", fstypename))
    {
        info->DeviceType = FILE_DEVICE_CD_ROM_FILE_SYSTEM;
        /* Don't assume read-only, let the mount options set it below */
        info->Characteristics |= FILE_REMOVABLE_MEDIA;
    }
    else if (!strcmp("nfs", fstypename) || !strcmp("nwfs", fstypename) ||
             !strcmp("smbfs", fstypename) || !strcmp("afpfs", fstypename))
    {
        info->DeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
        info->Characteristics |= FILE_REMOTE_DEVICE;
    }
    else if (!strcmp("procfs", fstypename))
        info->DeviceType = FILE_DEVICE_VIRTUAL_DISK;
    else
        info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;

    if (flags & MNT_RDONLY)
        info->Characteristics |= FILE_READ_ONLY_DEVICE;

    if (!(flags & MNT_LOCAL))
    {
        info->DeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
        info->Characteristics |= FILE_REMOTE_DEVICE;
    }
}
#endif

static inline BOOL is_device_placeholder( int fd )
{
    static const char wine_placeholder[] = "Wine device placeholder";
    char buffer[sizeof(wine_placeholder)-1];

    if (pread( fd, buffer, sizeof(wine_placeholder) - 1, 0 ) != sizeof(wine_placeholder) - 1)
        return FALSE;
    return !memcmp( buffer, wine_placeholder, sizeof(wine_placeholder) - 1 );
}

static NTSTATUS get_device_info( int fd, FILE_FS_DEVICE_INFORMATION *info )
{
    struct stat st;

    info->Characteristics = 0;
    if (fstat( fd, &st ) < 0) return errno_to_status( errno );
    if (S_ISCHR( st.st_mode ))
    {
        info->DeviceType = FILE_DEVICE_UNKNOWN;
#ifdef linux
        switch(major(st.st_rdev))
        {
        case MEM_MAJOR:
            info->DeviceType = FILE_DEVICE_NULL;
            break;
        case TTY_MAJOR:
            info->DeviceType = FILE_DEVICE_SERIAL_PORT;
            break;
        case LP_MAJOR:
            info->DeviceType = FILE_DEVICE_PARALLEL_PORT;
            break;
        case SCSI_TAPE_MAJOR:
            info->DeviceType = FILE_DEVICE_TAPE;
            break;
        }
#endif
    }
    else if (S_ISBLK( st.st_mode ))
    {
        info->DeviceType = FILE_DEVICE_DISK;
    }
    else if (S_ISFIFO( st.st_mode ) || S_ISSOCK( st.st_mode ))
    {
        info->DeviceType = FILE_DEVICE_NAMED_PIPE;
    }
    else if (is_device_placeholder( fd ))
    {
        info->DeviceType = FILE_DEVICE_DISK;
    }
    else  /* regular file or directory */
    {
#if defined(linux) && defined(HAVE_FSTATFS)
        struct statfs stfs;

        /* check for floppy disk */
        if (major(st.st_dev) == FLOPPY_MAJOR)
            info->Characteristics |= FILE_REMOVABLE_MEDIA;

        if (fstatfs( fd, &stfs ) < 0) stfs.f_type = 0;
        switch (stfs.f_type)
        {
        case 0x9660:      /* iso9660 */
        case 0x9fa1:      /* supermount */
        case 0x15013346:  /* udf */
            info->DeviceType = FILE_DEVICE_CD_ROM_FILE_SYSTEM;
            info->Characteristics |= FILE_REMOVABLE_MEDIA|FILE_READ_ONLY_DEVICE;
            break;
        case 0x6969:  /* nfs */
        case 0xff534d42: /* cifs */
        case 0xfe534d42: /* smb2 */
        case 0x517b:  /* smbfs */
        case 0x564c:  /* ncpfs */
            info->DeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
            info->Characteristics |= FILE_REMOTE_DEVICE;
            break;
        case 0x1373:      /* devfs */
        case 0x9fa0:      /* procfs */
            info->DeviceType = FILE_DEVICE_VIRTUAL_DISK;
            break;
        case 0x01021994:  /* tmpfs */
        case 0x28cd3d45:  /* cramfs */
            /* Don't map these to FILE_DEVICE_VIRTUAL_DISK by default. Virtual
             * filesystems are rare on Windows, and some programs refuse to
             * recognize them as valid. */
        default:
            info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
            break;
        }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
        struct statfs stfs;

        if (fstatfs( fd, &stfs ) < 0)
            info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
        else
            get_device_info_fstatfs( info, stfs.f_fstypename, stfs.f_flags );
#elif defined(__NetBSD__)
        struct statvfs stfs;

        if (fstatvfs( fd, &stfs) < 0)
            info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
        else
            get_device_info_fstatfs( info, stfs.f_fstypename, stfs.f_flag );
#elif defined(sun)
        /* Use dkio to work out device types */
        {
# include <sys/dkio.h>
# include <sys/vtoc.h>
            struct dk_cinfo dkinf;
            int retval = ioctl(fd, DKIOCINFO, &dkinf);
            if(retval==-1){
                WARN("Unable to get disk device type information - assuming a disk like device\n");
                info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
            }
            switch (dkinf.dki_ctype)
            {
            case DKC_CDROM:
                info->DeviceType = FILE_DEVICE_CD_ROM_FILE_SYSTEM;
                info->Characteristics |= FILE_REMOVABLE_MEDIA|FILE_READ_ONLY_DEVICE;
                break;
            case DKC_NCRFLOPPY:
            case DKC_SMSFLOPPY:
            case DKC_INTEL82072:
            case DKC_INTEL82077:
                info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
                info->Characteristics |= FILE_REMOVABLE_MEDIA;
                break;
            case DKC_MD:
            /* Don't map these to FILE_DEVICE_VIRTUAL_DISK by default. Virtual
             * filesystems are rare on Windows, and some programs refuse to
             * recognize them as valid. */
            default:
                info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
            }
        }
#else
        static int warned;
        if (!warned++) FIXME( "device info not properly supported on this platform\n" );
        info->DeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
#endif
        info->Characteristics |= FILE_DEVICE_IS_MOUNTED;
    }
    return STATUS_SUCCESS;
}


/******************************************************************************
 *              NtQueryVolumeInformationFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryVolumeInformationFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                              void *buffer, ULONG length,
                                              FS_INFORMATION_CLASS info_class )
{
    int fd, needs_close;
    struct stat st;

    io->u.Status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL );
    if (io->u.Status == STATUS_BAD_DEVICE_TYPE)
    {
        SERVER_START_REQ( get_volume_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->info_class = info_class;
            wine_server_set_reply( req, buffer, length );
            io->u.Status = wine_server_call( req );
            if (!io->u.Status) io->Information = wine_server_reply_size( reply );
        }
        SERVER_END_REQ;
        return io->u.Status;
    }
    else if (io->u.Status) return io->u.Status;

    io->u.Status = STATUS_NOT_IMPLEMENTED;
    io->Information = 0;

    switch( info_class )
    {
    case FileFsLabelInformation:
        FIXME( "%p: label info not supported\n", handle );
        break;

    case FileFsSizeInformation:
        if (length < sizeof(FILE_FS_SIZE_INFORMATION))
            io->u.Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            FILE_FS_SIZE_INFORMATION *info = buffer;

            if (fstat( fd, &st ) < 0)
            {
                io->u.Status = errno_to_status( errno );
                break;
            }
            if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            {
                io->u.Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                ULONGLONG bsize;
                /* Linux's fstatvfs is buggy */
#if !defined(linux) || !defined(HAVE_FSTATFS)
                struct statvfs stfs;

                if (fstatvfs( fd, &stfs ) < 0)
                {
                    io->u.Status = errno_to_status( errno );
                    break;
                }
                bsize = stfs.f_frsize;
#else
                struct statfs stfs;
                if (fstatfs( fd, &stfs ) < 0)
                {
                    io->u.Status = errno_to_status( errno );
                    break;
                }
                bsize = stfs.f_bsize;
#endif
                if (bsize == 2048)  /* assume CD-ROM */
                {
                    info->BytesPerSector = 2048;
                    info->SectorsPerAllocationUnit = 1;
                }
                else
                {
                    info->BytesPerSector = 512;
                    info->SectorsPerAllocationUnit = 8;
                }
                info->TotalAllocationUnits.QuadPart = bsize * stfs.f_blocks / (info->BytesPerSector * info->SectorsPerAllocationUnit);
                info->AvailableAllocationUnits.QuadPart = bsize * stfs.f_bavail / (info->BytesPerSector * info->SectorsPerAllocationUnit);
                io->Information = sizeof(*info);
                io->u.Status = STATUS_SUCCESS;
            }
        }
        break;

    case FileFsDeviceInformation:
        if (length < sizeof(FILE_FS_DEVICE_INFORMATION))
            io->u.Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            FILE_FS_DEVICE_INFORMATION *info = buffer;

            if ((io->u.Status = get_device_info( fd, info )) == STATUS_SUCCESS)
                io->Information = sizeof(*info);
        }
        break;

    case FileFsAttributeInformation:
    {
        static const WCHAR fatW[] = {'F','A','T'};
        static const WCHAR fat32W[] = {'F','A','T','3','2'};
        static const WCHAR ntfsW[] = {'N','T','F','S'};
        static const WCHAR cdfsW[] = {'C','D','F','S'};
        static const WCHAR udfW[] = {'U','D','F'};

        FILE_FS_ATTRIBUTE_INFORMATION *info = buffer;
        struct mountmgr_unix_drive drive;
        enum mountmgr_fs_type fs_type = MOUNTMGR_FS_TYPE_NTFS;

        if (length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
        {
            io->u.Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (!get_mountmgr_fs_info( handle, fd, &drive, sizeof(drive) )) fs_type = drive.fs_type;
        else
        {
            struct statfs stfs;

            if (!fstatfs( fd, &stfs ))
            {
#if defined(linux) && defined(HAVE_FSTATFS)
                switch (stfs.f_type)
                {
                case 0x9660:
                    fs_type = MOUNTMGR_FS_TYPE_ISO9660;
                    break;
                case 0x15013346:
                    fs_type = MOUNTMGR_FS_TYPE_UDF;
                    break;
                case 0x4d44:
                    fs_type = MOUNTMGR_FS_TYPE_FAT32;
                    break;
                }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
                if (!strcmp( stfs.f_fstypename, "cd9660" ))
                    fs_type = MOUNTMGR_FS_TYPE_ISO9660;
                else if (!strcmp( stfs.f_fstypename, "udf" ))
                    fs_type = MOUNTMGR_FS_TYPE_UDF;
                else if (!strcmp( stfs.f_fstypename, "msdos" ))
                    fs_type = MOUNTMGR_FS_TYPE_FAT32;
#endif
            }
        }

        switch (fs_type)
        {
        case MOUNTMGR_FS_TYPE_ISO9660:
            info->FileSystemAttributes = FILE_READ_ONLY_VOLUME;
            info->MaximumComponentNameLength = 221;
            info->FileSystemNameLength = min( sizeof(cdfsW), length - offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) );
            memcpy(info->FileSystemName, cdfsW, info->FileSystemNameLength);
            break;
        case MOUNTMGR_FS_TYPE_UDF:
            info->FileSystemAttributes = FILE_READ_ONLY_VOLUME | FILE_UNICODE_ON_DISK | FILE_CASE_SENSITIVE_SEARCH;
            info->MaximumComponentNameLength = 255;
            info->FileSystemNameLength = min( sizeof(udfW), length - offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) );
            memcpy(info->FileSystemName, udfW, info->FileSystemNameLength);
            break;
        case MOUNTMGR_FS_TYPE_FAT:
            info->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES; /* FIXME */
            info->MaximumComponentNameLength = 255;
            info->FileSystemNameLength = min( sizeof(fatW), length - offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) );
            memcpy(info->FileSystemName, fatW, info->FileSystemNameLength);
            break;
        case MOUNTMGR_FS_TYPE_FAT32:
            info->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES; /* FIXME */
            info->MaximumComponentNameLength = 255;
            info->FileSystemNameLength = min( sizeof(fat32W), length - offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) );
            memcpy(info->FileSystemName, fat32W, info->FileSystemNameLength);
            break;
        default:
            info->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES | FILE_PERSISTENT_ACLS;
            info->MaximumComponentNameLength = 255;
            info->FileSystemNameLength = min( sizeof(ntfsW), length - offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) );
            memcpy(info->FileSystemName, ntfsW, info->FileSystemNameLength);
            break;
        }

        io->Information = offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName ) + info->FileSystemNameLength;
        io->u.Status = STATUS_SUCCESS;
        break;
    }

    case FileFsVolumeInformation:
    {
        FILE_FS_VOLUME_INFORMATION *info = buffer;
        ULONGLONG data[64];
        struct mountmgr_unix_drive *drive = (struct mountmgr_unix_drive *)data;
        const WCHAR *label;

        if (length < sizeof(FILE_FS_VOLUME_INFORMATION))
        {
            io->u.Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (get_mountmgr_fs_info( handle, fd, drive, sizeof(data) ))
        {
            io->u.Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        label = (WCHAR *)((char *)drive + drive->label_offset);
        info->VolumeCreationTime.QuadPart = 0; /* FIXME */
        info->VolumeSerialNumber = drive->serial;
        info->VolumeLabelLength = min( wcslen( label ) * sizeof(WCHAR),
                                       length - offsetof( FILE_FS_VOLUME_INFORMATION, VolumeLabel ) );
        info->SupportsObjects = (drive->fs_type == MOUNTMGR_FS_TYPE_NTFS);
        memcpy( info->VolumeLabel, label, info->VolumeLabelLength );
        io->Information = offsetof( FILE_FS_VOLUME_INFORMATION, VolumeLabel ) + info->VolumeLabelLength;
        io->u.Status = STATUS_SUCCESS;
        break;
    }

    case FileFsControlInformation:
        FIXME( "%p: control info not supported\n", handle );
        break;

    case FileFsFullSizeInformation:
        FIXME( "%p: full size info not supported\n", handle );
        break;

    case FileFsObjectIdInformation:
        FIXME( "%p: object id info not supported\n", handle );
        break;

    case FileFsMaximumInformation:
        FIXME( "%p: maximum info not supported\n", handle );
        break;

    default:
        io->u.Status = STATUS_INVALID_PARAMETER;
        break;
    }
    if (needs_close) close( fd );
    return io->u.Status;
}


/******************************************************************************
 *              NtSetVolumeInformationFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetVolumeInformationFile( HANDLE handle, IO_STATUS_BLOCK *io, void *info,
                                            ULONG length, FS_INFORMATION_CLASS class )
{
    FIXME( "(%p,%p,%p,0x%08x,0x%08x) stub\n", handle, io, info, length, class );
    return STATUS_SUCCESS;
}


/******************************************************************
 *           NtQueryEaFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryEaFile( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                               BOOLEAN single_entry, void *list, ULONG list_len,
                               ULONG *index, BOOLEAN restart )
{
    FIXME( "(%p,%p,%p,%d,%d,%p,%d,%p,%d) stub\n",
           handle, io, buffer, length, single_entry, list, list_len, index, restart );
    return STATUS_ACCESS_DENIED;
}


/******************************************************************
 *           NtSetEaFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetEaFile( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length )
{
    FIXME( "(%p,%p,%p,%d) stub\n", handle, io, buffer, length );
    return STATUS_ACCESS_DENIED;
}


/**************************************************************************
 *           NtQueryObject   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryObject( HANDLE handle, OBJECT_INFORMATION_CLASS info_class,
                               void *ptr, ULONG len, ULONG *used_len )
{
    NTSTATUS status;

    TRACE("(%p,0x%08x,%p,0x%08x,%p)\n", handle, info_class, ptr, len, used_len);

    if (used_len) *used_len = 0;

    switch (info_class)
    {
    case ObjectBasicInformation:
    {
        OBJECT_BASIC_INFORMATION *p = ptr;

        if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

        SERVER_START_REQ( get_object_info )
        {
            req->handle = wine_server_obj_handle( handle );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                memset( p, 0, sizeof(*p) );
                p->GrantedAccess = reply->access;
                p->PointerCount = reply->ref_count;
                p->HandleCount = reply->handle_count;
                if (used_len) *used_len = sizeof(*p);
            }
        }
        SERVER_END_REQ;
        break;
    }

    case ObjectNameInformation:
    {
        OBJECT_NAME_INFORMATION *p = ptr;
        char *unix_name;
        WCHAR *nt_name;

        /* first try as a file object */

        if (!(status = server_get_unix_name( handle, &unix_name )))
        {
            if (!(status = unix_to_nt_file_name( unix_name, &nt_name )))
            {
                ULONG size = (wcslen(nt_name) + 1) * sizeof(WCHAR);
                if (len < sizeof(*p)) status = STATUS_INFO_LENGTH_MISMATCH;
                else if (len < sizeof(*p) + size) status = STATUS_BUFFER_OVERFLOW;
                else
                {
                    p->Name.Buffer = (WCHAR *)(p + 1);
                    p->Name.Length = size - sizeof(WCHAR);
                    p->Name.MaximumLength = size;
                    wcscpy( p->Name.Buffer, nt_name );
                }
                if (used_len) *used_len = sizeof(*p) + size;
                free( nt_name );
            }
            free( unix_name );
            break;
        }
        else if (status != STATUS_OBJECT_TYPE_MISMATCH) break;

        /* not a file, treat as a generic object */

        SERVER_START_REQ( get_object_info )
        {
            req->handle = wine_server_obj_handle( handle );
            if (len > sizeof(*p)) wine_server_set_reply( req, p + 1, len - sizeof(*p) );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                if (!reply->total)  /* no name */
                {
                    if (sizeof(*p) > len) status = STATUS_INFO_LENGTH_MISMATCH;
                    else memset( p, 0, sizeof(*p) );
                    if (used_len) *used_len = sizeof(*p);
                }
                else if (sizeof(*p) + reply->total + sizeof(WCHAR) > len)
                {
                    if (used_len) *used_len = sizeof(*p) + reply->total + sizeof(WCHAR);
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    ULONG res = wine_server_reply_size( reply );
                    p->Name.Buffer = (WCHAR *)(p + 1);
                    p->Name.Length = res;
                    p->Name.MaximumLength = res + sizeof(WCHAR);
                    p->Name.Buffer[res / sizeof(WCHAR)] = 0;
                    if (used_len) *used_len = sizeof(*p) + p->Name.MaximumLength;
                }
            }
        }
        SERVER_END_REQ;
        break;
    }

    case ObjectTypeInformation:
    {
        OBJECT_TYPE_INFORMATION *p = ptr;

        SERVER_START_REQ( get_object_type )
        {
            req->handle = wine_server_obj_handle( handle );
            if (len > sizeof(*p)) wine_server_set_reply( req, p + 1, len - sizeof(*p) );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                if (!reply->total)  /* no name */
                {
                    if (sizeof(*p) > len) status = STATUS_INFO_LENGTH_MISMATCH;
                    else memset( p, 0, sizeof(*p) );
                    if (used_len) *used_len = sizeof(*p);
                }
                else if (sizeof(*p) + reply->total + sizeof(WCHAR) > len)
                {
                    if (used_len) *used_len = sizeof(*p) + reply->total + sizeof(WCHAR);
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    ULONG res = wine_server_reply_size( reply );
                    p->TypeName.Buffer = (WCHAR *)(p + 1);
                    p->TypeName.Length = res;
                    p->TypeName.MaximumLength = res + sizeof(WCHAR);
                    p->TypeName.Buffer[res / sizeof(WCHAR)] = 0;
                    if (used_len) *used_len = sizeof(*p) + p->TypeName.MaximumLength;
                }
            }
        }
        SERVER_END_REQ;
        break;
    }

    case ObjectDataInformation:
    {
        OBJECT_DATA_INFORMATION* p = ptr;

        if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

        SERVER_START_REQ( set_handle_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->flags  = 0;
            req->mask   = 0;
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                p->InheritHandle = (reply->old_flags & HANDLE_FLAG_INHERIT) != 0;
                p->ProtectFromClose = (reply->old_flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;
                if (used_len) *used_len = sizeof(*p);
            }
        }
        SERVER_END_REQ;
        break;
    }

    default:
        FIXME("Unsupported information class %u\n", info_class);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    return status;
}


/**************************************************************************
 *           NtSetInformationObject   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationObject( HANDLE handle, OBJECT_INFORMATION_CLASS info_class,
                                        void *ptr, ULONG len )
{
    NTSTATUS status;

    TRACE("(%p,0x%08x,%p,0x%08x)\n", handle, info_class, ptr, len);

    switch (info_class)
    {
    case ObjectDataInformation:
    {
        OBJECT_DATA_INFORMATION* p = ptr;

        if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

        SERVER_START_REQ( set_handle_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->mask   = HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE;
            if (p->InheritHandle)    req->flags |= HANDLE_FLAG_INHERIT;
            if (p->ProtectFromClose) req->flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
    break;
    }

    default:
        FIXME("Unsupported information class %u\n", info_class);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    return status;
}
