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

#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif
#include <poll.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_ATTR_H
#include <sys/attr.h>
#endif
#ifdef MAJOR_IN_MKDEV
# include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
# include <sys/sysmacros.h>
#endif
#ifdef HAVE_SYS_VNODE_H
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
#ifdef HAVE_LINUX_IOCTL_H
#include <linux/ioctl.h>
#endif
#ifdef HAVE_LINUX_MAJOR_H
# include <linux/major.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_CONF_H
#include <sys/conf.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif
#ifdef HAVE_SYS_EXTATTR_H
#undef XATTR_ADDITIONAL_OPTIONS
#include <sys/extattr.h>
#endif
#include <time.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
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

#ifndef XATTR_USER_PREFIX
# define XATTR_USER_PREFIX "user."
#endif
#ifndef XATTR_USER_PREFIX_LEN
# define XATTR_USER_PREFIX_LEN (sizeof(XATTR_USER_PREFIX) - 1)
#endif

#define SAMBA_XATTR_DOS_ATTRIB  XATTR_USER_PREFIX "DOSATTRIB"
#define XATTR_ATTRIBS_MASK      (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)

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
    UNICODE_STRING          mask;    /* the mask used when creating the cache entry */
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

static BOOL is_wildcard( WCHAR c )
{
    return c == '*' || c == '?' || c == '>' || c == '<' || c == '\"';
}

static inline BOOL has_wildcard( const UNICODE_STRING *mask )
{
    int i;

    if (!mask) return TRUE;
    for (i = 0; i < mask->Length / sizeof(WCHAR); i++)
        if (is_wildcard( mask->Buffer[i] )) return TRUE;

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
    case EISDIR:    return STATUS_INVALID_DEVICE_REQUEST;
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


static int xattr_fremove( int filedes, const char *name )
{
#ifdef HAVE_SYS_XATTR_H
# ifdef XATTR_ADDITIONAL_OPTIONS
    return fremovexattr( filedes, name, 0 );
# else
    return fremovexattr( filedes, name );
# endif
#elif defined(HAVE_SYS_EXTATTR_H)
    return extattr_delete_fd( filedes, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN] );
#else
    errno = ENOSYS;
    return -1;
#endif
}


static int xattr_fset( int filedes, const char *name, const void *value, size_t size )
{
#ifdef HAVE_SYS_XATTR_H
# ifdef XATTR_ADDITIONAL_OPTIONS
    return fsetxattr( filedes, name, value, size, 0, 0 );
# else
    return fsetxattr( filedes, name, value, size, 0 );
# endif
#elif defined(HAVE_SYS_EXTATTR_H)
    return extattr_set_fd( filedes, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN],
                           value, size );
#else
    errno = ENOSYS;
    return -1;
#endif
}


/* On macOS, getxattr() is significantly slower than listxattr()
 * (even for files with no extended attributes).
 */
#ifdef __APPLE__
static BOOL xattr_exists( const char **path, int *filedes, const char *name )
{
    char xattrs[1024];
    ssize_t i = 0, ret;

    if (path)
        ret = listxattr( *path, xattrs, sizeof(xattrs), 0 );
    else
        ret = flistxattr( *filedes, xattrs, sizeof(xattrs), 0 );
    if (ret == -1)
        return errno == ERANGE;

    while (i < ret)
    {
        if (!strcmp( name, &xattrs[i] ))
            return TRUE;
        i += strlen(&xattrs[i]) + 1;
    }

    errno = ENOATTR;
    return FALSE;
}
#endif


static int xattr_get( const char *path, const char *name, void *value, size_t size )
{
#ifdef __APPLE__
    if (!xattr_exists( &path, NULL, name ))
        return -1;
#endif

#ifdef HAVE_SYS_XATTR_H
# ifdef XATTR_ADDITIONAL_OPTIONS
    return getxattr( path, name, value, size, 0, 0 );
# else
    return getxattr( path, name, value, size );
# endif
#elif defined(HAVE_SYS_EXTATTR_H)
    return extattr_get_file( path, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN],
                             value, size );
#else
    errno = ENOSYS;
    return -1;
#endif
}


static int xattr_fget( int filedes, const char *name, void *value, size_t size )
{
#ifdef __APPLE__
    if (!xattr_exists( NULL, &filedes, name ))
        return -1;
#endif

#ifdef HAVE_SYS_XATTR_H
# ifdef XATTR_ADDITIONAL_OPTIONS
    return fgetxattr( filedes, name, value, size, 0, 0 );
# else
    return fgetxattr( filedes, name, value, size );
# endif
#elif defined(HAVE_SYS_EXTATTR_H)
    return extattr_get_fd( filedes, EXTATTR_NAMESPACE_USER, &name[XATTR_USER_PREFIX_LEN],
                           value, size );
#else
    errno = ENOSYS;
    return -1;
#endif
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
    free( data->mask.Buffer );
    free( data );
}


/* support for a directory queue for filesystem searches */

struct dir_name
{
    struct list entry;
    char name[1];
};

static NTSTATUS add_dir_to_queue( struct list *queue, const char *name )
{
    int len = strlen( name ) + 1;
    struct dir_name *dir = malloc( offsetof( struct dir_name, name[len] ));
    if (!dir) return STATUS_NO_MEMORY;
    strcpy( dir->name, name );
    list_add_tail( queue, &dir->entry );
    return STATUS_SUCCESS;
}

static NTSTATUS next_dir_in_queue( struct list *queue, char *name )
{
    struct list *head = list_head( queue );
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

static void flush_dir_queue( struct list *queue )
{
    struct list *head;

    while ((head = list_head( queue )))
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
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
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

static pthread_mutex_t fs_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

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
 * sensitive or not. Uses getattrlist(2)/getattrlistat(2).
 */
static int get_dir_case_sensitivity_attr( int root_fd, const char *dir )
{
    BOOLEAN ret = FALSE;
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
    if (getattrlistat( root_fd, dir, &attr, &get_fsid, sizeof(get_fsid), 0 ) != 0 ||
        get_fsid.size != sizeof(get_fsid))
        return -1;

    /* Try to look it up in the cache */
    mutex_lock( &fs_cache_mutex );
    entry = look_up_fs_cache( get_fsid.dev );
    if (entry && !memcmp( &entry->fsid, &get_fsid.fsid, sizeof(fsid_t) ))
    {
        /* Cache lookup succeeded */
        ret = entry->case_sensitive;
        goto done;
    }

    /* Cache is stale at this point, we have to update it */
    mntpoint = get_device_mount_point( get_fsid.dev );
    /* Now look up the case-sensitivity */
    attr.commonattr = 0;
    attr.volattr = ATTR_VOL_INFO|ATTR_VOL_CAPABILITIES;
    if (getattrlist( mntpoint, &attr, &caps, sizeof(caps), 0 ) < 0)
    {
        free( mntpoint );
        add_fs_cache( get_fsid.dev, get_fsid.fsid, TRUE );
        ret = TRUE;
        goto done;
    }
    free( mntpoint );
    if (caps.size == sizeof(caps) &&
        (caps.caps.valid[VOL_CAPABILITIES_FORMAT] &
         (VOL_CAP_FMT_CASE_SENSITIVE | VOL_CAP_FMT_CASE_PRESERVING)) ==
        (VOL_CAP_FMT_CASE_SENSITIVE | VOL_CAP_FMT_CASE_PRESERVING))
    {
        if ((caps.caps.capabilities[VOL_CAPABILITIES_FORMAT] &
            VOL_CAP_FMT_CASE_SENSITIVE) != VOL_CAP_FMT_CASE_SENSITIVE)
            ret = FALSE;
        else
            ret = TRUE;
        /* Update the cache */
        add_fs_cache( get_fsid.dev, get_fsid.fsid, ret );
        goto done;
    }

done:
    mutex_unlock( &fs_cache_mutex );
    return ret;
}
#endif

/***********************************************************************
 *           get_dir_case_sensitivity_stat
 *
 * Checks if the volume containing the specified directory is case
 * sensitive or not. Uses (f)statfs(2), statvfs(2), fstatat(2), or ioctl(2).
 */
static BOOLEAN get_dir_case_sensitivity_stat( int root_fd, const char *dir )
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    struct statfs stfs;
    int fd;

    if ((fd = openat( root_fd, dir, O_RDONLY )) == -1) return TRUE;
    if (fstatfs( fd, &stfs ) == -1)
    {
        close( fd );
        return TRUE;
    }
    close( fd );
    /* Assume these file systems are always case insensitive.*/
    if (!strcmp( stfs.f_fstypename, "fusefs" ) &&
        !strncmp( stfs.f_mntfromname, "ciopfs", 5 ))
        return FALSE;
    /* msdosfs was case-insensitive since FreeBSD 8, if not earlier */
    if (!strcmp( stfs.f_fstypename, "msdosfs" ) ||
        /* older CIFS protocol versions uppercase filename on the client,
         * newer versions should be case-insensitive on the server anyway */
        !strcmp( stfs.f_fstypename, "smbfs" ))
        return FALSE;
    /* no ntfs-3g: modern fusefs has no way to report the filesystem on FreeBSD
     * no cd9660 or udf, they're case-sensitive on FreeBSD
     */
#ifdef __APPLE__
    if (!strcmp( stfs.f_fstypename, "msdos" ) ||
        !strcmp( stfs.f_fstypename, "cd9660" ) ||
        !strcmp( stfs.f_fstypename, "udf" ) ||
        !strcmp( stfs.f_fstypename, "ntfs" ))
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
    int fd;

    if ((fd = openat( root_fd, dir, O_RDONLY )) == -1) return TRUE;
    if (fstatvfs( fd, &stfs ) == -1)
    {
        close( fd );
        return TRUE;
    }
    close( fd );
    /* Only assume CIOPFS is case insensitive. */
    if (strcmp( stfs.f_fstypename, "fusefs" ) ||
        strncmp( stfs.f_mntfromname, "ciopfs", 5 ))
        return FALSE;
    return TRUE;

#elif defined(__linux__)
    BOOLEAN sens = TRUE;
    struct statfs stfs;
    struct stat st;
    int fd, flags;

    if ((fd = openat( root_fd, dir, O_RDONLY | O_NONBLOCK )) == -1)
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
static BOOLEAN get_dir_case_sensitivity( int root_fd, const char *dir )
{
#if defined(HAVE_GETATTRLIST) && defined(ATTR_VOL_CAPABILITIES) && \
    defined(VOL_CAPABILITIES_FORMAT) && defined(VOL_CAP_FMT_CASE_SENSITIVE)
    int case_sensitive = get_dir_case_sensitivity_attr( root_fd, dir );
    if (case_sensitive != -1) return case_sensitive;
#endif
    return get_dir_case_sensitivity_stat( root_fd, dir );
}


/***********************************************************************
 *           is_hidden_file
 *
 * Check if the specified file should be hidden based on its unix path and the show dot files option.
 */
static BOOL is_hidden_file( const char *name )
{
    const char *p;

    if (show_dot_files) return FALSE;

    p = name + strlen( name );
    while (p > name && p[-1] == '/') p--;
    while (p > name && p[-1] != '/') p--;
    if (*p++ != '.') return FALSE;
    if (!*p || *p == '/') return FALSE;  /* "." directory */
    if (*p++ != '.') return TRUE;
    if (!*p || *p == '/') return FALSE;  /* ".." directory */
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
    p = name;
    while (*p == '.') ++p;
    for (p = p + 1, ext = NULL; p < end - 1; p++) if (*p == '.') ext = p;

    /* Copy first 4 chars, replacing invalid chars with '_' */
    for (i = 4, p = name, dst = buffer; i > 0; p++)
    {
        if (p == end || p == ext) break;
        if (*p == '.') continue;
        *dst++ = is_invalid_dos_char(*p) ? '_' : *p;
        i--;
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
 *           match_filename_part
 *
 * Recursive helper for match_filename().
 *
 */
static BOOLEAN match_filename_part( const WCHAR *name, const WCHAR *name_end, const WCHAR *mask, const WCHAR *mask_end )
{
    WCHAR c;

    while (name < name_end && mask < mask_end)
    {
        switch(*mask)
        {
        case '*':
            mask++;
            while (mask < mask_end && *mask == '*') mask++;  /* Skip consecutive '*' */
            if (mask == mask_end) return TRUE; /* end of mask is all '*', so match */

            while (name < name_end)
            {
                c = *mask == '"' ? '.' : *mask;
                if (!is_wildcard(c))
                {
                    if (is_case_sensitive)
                        while (name < name_end && (*name != c)) name++;
                    else
                        while (name < name_end && (towupper(*name) != towupper(c))) name++;
                }
                if (match_filename_part( name, name_end, mask, mask_end )) return TRUE;
                ++name;
            }
            break;
        case '<':
        {
            const WCHAR *next_dot;
            BOOL had_dot = FALSE;

            ++mask;
            while (name < name_end)
            {
                next_dot = name;
                while (next_dot < name_end && *next_dot != '.') ++next_dot;
                if (next_dot == name_end && had_dot) break;
                if (next_dot < name_end)
                {
                    had_dot = TRUE;
                    ++next_dot;
                }
                if (mask < mask_end)
                {
                    while (name < next_dot)
                    {
                        c = *mask == '"' ? '.' : *mask;
                        if (!is_wildcard(c))
                        {
                            if (is_case_sensitive)
                                while (name < next_dot && (*name != c)) name++;
                            else
                                while (name < next_dot && (towupper(*name) != towupper(c))) name++;
                        }
                        if (match_filename_part( name, name_end, mask, mask_end )) return TRUE;
                        ++name;
                    }
                }
                name = next_dot;
            }
            break;
        }
        case '?':
            mask++;
            name++;
            break;
        case '>':
            mask++;
            if (*name == '.')
            {
                while (mask < mask_end && *mask == '>') mask++;
                if (mask == mask_end) name++;
            }
            else name++;
            break;
        default:
            c = *mask == '"' ? '.' : *mask;
            if (is_case_sensitive && c != *name) return FALSE;
            if (!is_case_sensitive && towupper(c) != towupper(*name)) return FALSE;
            mask++;
            name++;
            break;
        }
    }
    while (mask < mask_end && (*mask == '*' || *mask == '<' || *mask == '"' || *mask == '>'))
        mask++;
    return (name == name_end && mask == mask_end);
}


/***********************************************************************
 *           match_filename
 *
 * Check a file name against a mask.
 *
 */
static BOOLEAN match_filename( const WCHAR *name, int length, const UNICODE_STRING *mask_str )
{
    /* Special handling for parent directory. */
    if (length == 2 && name[0] == '.' && name[1] == '.') --length;

    return match_filename_part( name, name + length, mask_str->Buffer,
                                mask_str->Buffer + mask_str->Length / sizeof(WCHAR));
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


/* decode the xattr-stored DOS attributes */
static int parse_samba_dos_attrib_data( char *data, int len )
{
    char *end;
    int val;

    if (len > 2 && data[0] == '0' && data[1] == 'x')
    {
        data[len] = 0;
        val = strtol( data, &end, 16 );
        if (!*end) return val & XATTR_ATTRIBS_MASK;
    }
    else
    {
        static BOOL once;
        if (!once++) FIXME( "Unhandled " SAMBA_XATTR_DOS_ATTRIB " extended attribute value.\n" );
    }
    return 0;
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
    char attr_data[65];
    int attr_len, ret;

    *attr = 0;
    ret = fstat( fd, st );
    if (ret == -1) return ret;
    *attr |= get_file_attributes( st );
    /* consider mount points to be reparse points (IO_REPARSE_TAG_MOUNT_POINT) */
    if ((options & FILE_OPEN_REPARSE_POINT) && fd_is_mount_point( fd, st ))
        *attr |= FILE_ATTRIBUTE_REPARSE_POINT;

    attr_len = xattr_fget( fd, SAMBA_XATTR_DOS_ATTRIB, attr_data, sizeof(attr_data)-1 );
    if (attr_len != -1)
        *attr |= parse_samba_dos_attrib_data( attr_data, attr_len );
    else
    {
        if (errno == ENOTSUP) return ret;
#ifdef ENODATA
        if (errno == ENODATA) return ret;
#endif
#ifdef ENOATTR
        if (errno == ENOATTR) return ret;
#endif
        WARN( "Failed to get extended attribute " SAMBA_XATTR_DOS_ATTRIB ". errno %d (%s)\n",
              errno, strerror( errno ) );
    }
    return ret;
}


static int fd_set_dos_attrib( int fd, UINT attr, BOOL force_set )
{
    /* we only store the HIDDEN and SYSTEM attributes */
    attr &= XATTR_ATTRIBS_MASK;
    if (force_set || attr != 0)
    {
        /* encode the attributes in Samba 3 ASCII format. Samba 4 has extended
         * this format with more features, but retains compatibility with the
         * earlier format. */
        char data[11];
        int len = snprintf( data, sizeof(data), "0x%x", attr );
        return xattr_fset( fd, SAMBA_XATTR_DOS_ATTRIB, data, len );
    }
    else return xattr_fremove( fd, SAMBA_XATTR_DOS_ATTRIB );
}


/* set the stat info and file attributes for a file (by file descriptor) */
static NTSTATUS fd_set_file_info( int fd, UINT attr, BOOL force_set_xattr )
{
    struct stat st;

    if (fstat( fd, &st ) == -1) return errno_to_status( errno );
    if (attr & FILE_ATTRIBUTE_READONLY)
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
    if (fchmod( fd, st.st_mode ) == -1) return errno_to_status( errno );

    /* if the file has multiple names, we can't be sure that it is safe to not
       set the extended attribute, since any of the names could start with a dot */
    force_set_xattr = force_set_xattr || st.st_nlink > 1;

    if (fd_set_dos_attrib( fd, attr, force_set_xattr ) == -1 && errno != ENOTSUP)
        WARN( "Failed to set extended attribute " SAMBA_XATTR_DOS_ATTRIB ". errno %d (%s)\n",
              errno, strerror( errno ) );

    return STATUS_SUCCESS;
}


/* get the stat info and file attributes for a file (by name) */
static int get_file_info( const char *path, struct stat *st, ULONG *attr )
{
    char *parent_path;
    char attr_data[65];
    int attr_len, ret;

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

    attr_len = xattr_get( path, SAMBA_XATTR_DOS_ATTRIB, attr_data, sizeof(attr_data)-1 );
    if (attr_len != -1)
        *attr |= parse_samba_dos_attrib_data( attr_data, attr_len );
    else
    {
        if (is_hidden_file( path ))
            *attr |= FILE_ATTRIBUTE_HIDDEN;
        if (errno == ENOTSUP) return ret;
#ifdef ENODATA
        if (errno == ENODATA) return ret;
#endif
#ifdef ENOATTR
        if (errno == ENOATTR) return ret;
#endif
        WARN( "Failed to get extended attribute " SAMBA_XATTR_DOS_ATTRIB " from %s. errno %d (%s)\n",
              debugstr_a(path), errno, strerror( errno ) );
    }
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
    mtime->QuadPart = ticks_from_time_t( st->st_mtime );
    ctime->QuadPart = ticks_from_time_t( st->st_ctime );
    atime->QuadPart = ticks_from_time_t( st->st_atime );
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
    creation->QuadPart = ticks_from_time_t( st->st_birthtime );
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIM
    creation->QuadPart += st->st_birthtim.tv_nsec / 100;
#elif defined(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC)
    creation->QuadPart += st->st_birthtimespec.tv_nsec / 100;
#endif
#elif defined(HAVE_STRUCT_STAT___ST_BIRTHTIME)
    creation->QuadPart = ticks_from_time_t( st->__st_birthtime );
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
    case FileNetworkOpenInformation:
        {
            FILE_NETWORK_OPEN_INFORMATION *info = ptr;
            get_file_times( st, &info->LastWriteTime, &info->ChangeTime,
                            &info->LastAccessTime, &info->CreationTime );
            info->FileAttributes = attr;
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


static unsigned int server_get_unix_name( HANDLE handle, char **unix_name )
{
    data_size_t size = 1024;
    unsigned int ret;
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

static NTSTATUS server_get_name_info( HANDLE handle, FILE_NAME_INFORMATION *info, LONG *name_len )
{
    data_size_t size = 1024;
    NTSTATUS status;
    OBJECT_NAME_INFORMATION *name;

    for (;;)
    {
        if (!(name = malloc( size ))) return STATUS_NO_MEMORY;
        if (!(status = NtQueryObject( handle, ObjectNameInformation, name, size, &size )))
        {
            const WCHAR *ptr = name->Name.Buffer;
            const WCHAR *end = ptr + name->Name.Length / sizeof(WCHAR);

            /* Skip the volume mount point. */
            while (ptr != end && *ptr == '\\') ++ptr;
            while (ptr != end && *ptr != '\\') ++ptr;
            while (ptr != end && *ptr == '\\') ++ptr;
            while (ptr != end && *ptr != '\\') ++ptr;

            info->FileNameLength = (end - ptr) * sizeof(WCHAR);
            if (*name_len < info->FileNameLength) status = STATUS_BUFFER_OVERFLOW;
            else if (!info->FileNameLength) status = STATUS_INVALID_INFO_CLASS;
            else *name_len = info->FileNameLength;
            memcpy( info->FileName, ptr, *name_len );
            free( name );
        }
        else
        {
            free( name );
            if (status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_BUFFER_OVERFLOW) continue;
        }
        return status;
    }
}


static NTSTATUS get_full_size_info(int fd, FILE_FS_FULL_SIZE_INFORMATION *info) {
    struct stat st;
    ULONGLONG bsize;

#if !defined(linux) || !defined(HAVE_FSTATFS)
    struct statvfs stfs;
#else
    struct statfs stfs;
#endif

    if (fstat( fd, &st ) < 0) return errno_to_status( errno );
    if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) return STATUS_INVALID_DEVICE_REQUEST;

    /* Linux's fstatvfs is buggy */
#if !defined(linux) || !defined(HAVE_FSTATFS)
    if (fstatvfs( fd, &stfs ) < 0) return errno_to_status( errno );
    bsize = stfs.f_frsize;
#else
    if (fstatfs( fd, &stfs ) < 0) return errno_to_status( errno );
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
    info->CallerAvailableAllocationUnits.QuadPart = bsize * stfs.f_bavail / (info->BytesPerSector * info->SectorsPerAllocationUnit);
    info->ActualAvailableAllocationUnits.QuadPart = bsize * stfs.f_bfree / (info->BytesPerSector * info->SectorsPerAllocationUnit);
    return STATUS_SUCCESS;
}

static NTSTATUS get_full_size_info_ex(int fd, FILE_FS_FULL_SIZE_INFORMATION_EX *info)
{
    FILE_FS_FULL_SIZE_INFORMATION full_info;
    NTSTATUS status;

    if ((status = get_full_size_info(fd, &full_info)) != STATUS_SUCCESS)
        return status;

    info->ActualTotalAllocationUnits = full_info.TotalAllocationUnits.QuadPart;
    info->ActualAvailableAllocationUnits = full_info.ActualAvailableAllocationUnits.QuadPart;
    info->ActualPoolUnavailableAllocationUnits = 0;
    info->CallerAvailableAllocationUnits = full_info.CallerAvailableAllocationUnits.QuadPart;
    info->CallerPoolUnavailableAllocationUnits = 0;
    info->UsedAllocationUnits = info->ActualTotalAllocationUnits - info->ActualAvailableAllocationUnits;
    info->CallerTotalAllocationUnits = info->CallerAvailableAllocationUnits + info->UsedAllocationUnits;
    info->TotalReservedAllocationUnits = 0;
    info->VolumeStorageReserveAllocationUnits = 0;
    info->AvailableCommittedAllocationUnits = 0;
    info->PoolAvailableAllocationUnits = 0;
    info->SectorsPerAllocationUnit = full_info.SectorsPerAllocationUnit;
    info->BytesPerSector = full_info.BytesPerSector;

    return STATUS_SUCCESS;
}


static NTSTATUS server_get_file_info( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer,
                                      ULONG length, FILE_INFORMATION_CLASS info_class )
{
    SERVER_START_REQ( get_file_info )
    {
        req->handle = wine_server_obj_handle( handle );
        req->info_class = info_class;
        wine_server_set_reply( req, buffer, length );
        io->Status = wine_server_call( req );
        io->Information = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;
    if (io->Status == STATUS_NOT_IMPLEMENTED)
        FIXME( "Unsupported info class %x\n", info_class );
    return io->Status;

}


static unsigned int server_open_file_object( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                             ULONG sharing, ULONG options )
{
    unsigned int status;

    SERVER_START_REQ( open_file_object )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        req->sharing    = sharing;
        req->options    = options;
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        status = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return status;
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

        if (asprintf( &buffer, "%s/dosdevices/a:", config_dir ) != -1)
        {
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
    HANDLE mountmgr;
    unsigned int status;
    int letter = -1;

    if (!server_get_unix_name( handle, &unix_name ))
    {
        letter = find_dos_device( unix_name );
        free( unix_name );
    }
    memset( drive, 0, sizeof(*drive) );
    if (letter == -1)
    {
        struct stat st;

        fstat( fd, &st );
        drive->unix_dev = st.st_rdev ? st.st_rdev : st.st_dev;
    }
    else
        drive->letter = 'a' + letter;

    init_unicode_string( &string, MOUNTMGR_DEVICE_NAME );
    InitializeObjectAttributes( &attr, &string, 0, NULL, NULL );
    status = server_open_file_object( &mountmgr, GENERIC_READ | SYNCHRONIZE, &attr,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT );
    if (status) return status;

    status = sync_ioctl( mountmgr, IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE, drive, sizeof(*drive), drive, size );
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
        TRACE( "file no longer exists %s\n", debugstr_a(names->unix_name) );
        return STATUS_SUCCESS;
    }
    if (is_ignored_file( &st ))
    {
        TRACE( "ignoring file %s\n", debugstr_a(names->unix_name) );
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
#pragma pack(push,4)
    struct
    {
        u_int32_t length;
        struct attrreference name_reference;
        fsobj_type_t type;
        char name[NAME_MAX * 3 + 1];
    } buffer;
#pragma pack(pop)

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

    TRACE( "found %s\n", debugstr_a(buffer.name) );

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
    if (!get_dir_case_sensitivity( AT_FDCWD, "." )) return STATUS_NO_SUCH_FILE;
    if (stat( unix_name, &st ) == -1) return STATUS_NO_SUCH_FILE;

    TRACE( "found %s\n", debugstr_a(unix_name) );

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

    if (mask)
    {
        data->mask.Length = data->mask.MaximumLength = mask->Length;
        if (!(data->mask.Buffer = malloc( mask->Length )))
        {
            free_dir_data( data );
            return STATUS_NO_MEMORY;
        }
        memcpy(data->mask.Buffer, mask->Buffer, mask->Length);
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
 *           ustring_equal
 *
 * Simplified version of RtlEqualUnicodeString that performs only case-sensitive comparisons.
 */
static BOOLEAN ustring_equal( const UNICODE_STRING *a, const UNICODE_STRING *b )
{
    USHORT length_a = (a ? a->Length : 0);
    USHORT length_b = (b ? b->Length : 0);

    if (length_a != length_b) return FALSE;
    if (length_a == 0) return TRUE;
    return !memcmp(a->Buffer, b->Buffer, a->Length);
}


/***********************************************************************
 *           get_cached_dir_data
 *
 * Retrieve the cached directory data, or initialize it if necessary.
 */
static unsigned int get_cached_dir_data( HANDLE handle, struct dir_data **data_ret, int fd,
                                         const UNICODE_STRING *mask, BOOLEAN restart_scan )
{
    unsigned int i;
    int entry = -1, free_entries[16];
    unsigned int status;
    BOOLEAN fresh_handle;

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

    fresh_handle = !dir_data_cache[entry];

    if (dir_data_cache[entry] && restart_scan && mask &&
        !ustring_equal(&dir_data_cache[entry]->mask, mask))
    {
        TRACE( "invalidating existing cache entry for handle %p, old mask: \"%s\", new mask: \"%s\"\n",
               handle, debugstr_us(&(dir_data_cache[entry]->mask)), debugstr_us(mask));
        free_dir_data( dir_data_cache[entry] );
        dir_data_cache[entry] = NULL;
    }

    if (!dir_data_cache[entry])
    {
        status = init_cached_dir_data( &dir_data_cache[entry], fd, mask );
        if (status == STATUS_NO_SUCH_FILE && !fresh_handle) status = STATUS_NO_MORE_FILES;
    }

    *data_ret = dir_data_cache[entry];
    if (restart_scan) (*data_ret)->pos = 0;
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
    unsigned int status;

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
    if (mask && mask->Length == 0) mask = NULL;

    mutex_lock( &dir_mutex );

    cwd = open( ".", O_RDONLY );
    if (fchdir( fd ) != -1)
    {
        if (!(status = get_cached_dir_data( handle, &data, fd, mask, restart_scan )))
        {
            union file_directory_info *last_info = NULL;

            while (!status && data->pos < data->count)
            {
                status = get_dir_data_entry( data, buffer, io, length, info_class, &last_info );
                if (!status || status == STATUS_BUFFER_OVERFLOW) data->pos++;
                if (single_entry && last_info) break;
            }

            if (!last_info) status = STATUS_NO_MORE_FILES;
            else if (status == STATUS_MORE_ENTRIES) status = STATUS_SUCCESS;
        }
        if (cwd == -1 || fchdir( cwd ) == -1) chdir( "/" );
    }
    else status = errno_to_status( errno );

    if (status != STATUS_NO_SUCH_FILE) io->Status = status;

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
static NTSTATUS find_file_in_dir( int root_fd, char *unix_name, int pos, const WCHAR *name, int length,
                                  BOOLEAN check_case )
{
    WCHAR buffer[MAX_DIR_ENTRY_LEN];
    BOOLEAN is_name_8_dot_3;
    DIR *dir;
    struct dirent *de;
    struct stat st;
    int fd, ret;

    /* try a shortcut for this directory */

    unix_name[pos++] = '/';
    ret = ntdll_wcstoumbs( name, length, unix_name + pos, MAX_DIR_ENTRY_LEN + 1, TRUE );
    if (ret >= 0 && ret <= MAX_DIR_ENTRY_LEN)
    {
        unix_name[pos + ret] = 0;
        if (!fstatat( root_fd, unix_name, &st, 0 )) return STATUS_SUCCESS;
    }
    if (check_case) goto not_found;  /* we want an exact match */

    if (pos > 1) unix_name[pos - 1] = 0;
    else unix_name[1] = 0;  /* keep the initial slash */

    /* check if it fits in 8.3 so that we don't look for short names if we won't need them */

    is_name_8_dot_3 = is_legal_8dot3_name( name, length );
#ifndef VFAT_IOCTL_READDIR_BOTH
    is_name_8_dot_3 = is_name_8_dot_3 && length >= 8 && name[4] == '~';
#endif

    if (!is_name_8_dot_3 && !get_dir_case_sensitivity( root_fd, unix_name )) goto not_found;

    /* now look for it through the directory */

#ifdef VFAT_IOCTL_READDIR_BOTH
    if (is_name_8_dot_3)
    {
        int fd = openat( root_fd, unix_name, O_RDONLY | O_DIRECTORY );
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
                            return STATUS_SUCCESS;
                        }
                    }
                    ret = ntdll_umbstowcs( kde[0].d_name, strlen(kde[0].d_name),
                                           buffer, MAX_DIR_ENTRY_LEN );
                    if (ret == length && !wcsnicmp( buffer, name, ret ))
                    {
                        strcpy( unix_name + pos,
                                kde[1].d_name[0] ? kde[1].d_name : kde[0].d_name );
                        close( fd );
                        return STATUS_SUCCESS;
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

    if ((fd = openat( root_fd, unix_name, O_RDONLY )) == -1) return errno_to_status( errno );
    if (!(dir = fdopendir( fd )))
    {
        close( fd );
        return errno_to_status( errno );
    }

    unix_name[pos - 1] = '/';
    while ((de = readdir( dir )))
    {
        ret = ntdll_umbstowcs( de->d_name, strlen(de->d_name), buffer, MAX_DIR_ENTRY_LEN );
        if (ret == length && !wcsnicmp( buffer, name, ret ))
        {
            strcpy( unix_name + pos, de->d_name );
            closedir( dir );
            return STATUS_SUCCESS;
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
                return STATUS_SUCCESS;
            }
        }
    }
    closedir( dir );

not_found:
    unix_name[pos - 1] = 0;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}


#ifndef _WIN64

static const WCHAR catrootW[] = {'s','y','s','t','e','m','3','2','\\','c','a','t','r','o','o','t',0};
static const WCHAR catroot2W[] = {'s','y','s','t','e','m','3','2','\\','c','a','t','r','o','o','t','2',0};
static const WCHAR driversstoreW[] = {'s','y','s','t','e','m','3','2','\\','d','r','i','v','e','r','s','s','t','o','r','e',0};
static const WCHAR driversetcW[] = {'s','y','s','t','e','m','3','2','\\','d','r','i','v','e','r','s','\\','e','t','c',0};
static const WCHAR logfilesW[] = {'s','y','s','t','e','m','3','2','\\','l','o','g','f','i','l','e','s',0};
static const WCHAR spoolW[] = {'s','y','s','t','e','m','3','2','\\','s','p','o','o','l',0};
static const WCHAR system32W[] = {'s','y','s','t','e','m','3','2',0};
static const WCHAR syswow64W[] = {'s','y','s','w','o','w','6','4',0};
static const WCHAR sysnativeW[] = {'s','y','s','n','a','t','i','v','e',0};
static const WCHAR regeditW[] = {'r','e','g','e','d','i','t','.','e','x','e',0};
static const WCHAR syswow64_regeditW[] = {'s','y','s','w','o','w','6','4','\\','r','e','g','e','d','i','t','.','e','x','e',0};
static const WCHAR windirW[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',0};
static const WCHAR syswow64dirW[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\','s','y','s','w','o','w','6','4','\\'};

static const WCHAR * const no_redirect[] =
{
    catrootW,
    catroot2W,
    driversstoreW,
    driversetcW,
    logfilesW,
    spoolW
};

static struct file_identity windir, sysdir;

static inline ULONG starts_with_path( const WCHAR *name, ULONG name_len, const WCHAR *prefix )
{
    ULONG len = wcslen( prefix );

    if (name_len < len) return 0;
    if (wcsnicmp( name, prefix, len )) return 0;
    if (name_len > len && name[len] != '\\') return 0;
    return len;
}

static BOOL replace_path( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *str, ULONG prefix_len,
                          const WCHAR *match, const WCHAR *replace )
{
    const WCHAR *name = attr->ObjectName->Buffer;
    ULONG match_len, replace_len, len = attr->ObjectName->Length / sizeof(WCHAR);
    WCHAR *p;

    if (!starts_with_path( name + prefix_len, len - prefix_len, match )) return FALSE;

    match_len = wcslen( match );
    replace_len = wcslen( replace );
    str->Length = (len + replace_len - match_len) * sizeof(WCHAR);
    str->MaximumLength = str->Length + sizeof(WCHAR);
    if (!(p = str->Buffer = malloc( str->MaximumLength ))) return FALSE;

    memcpy( p, name, prefix_len * sizeof(WCHAR) );
    p += prefix_len;
    memcpy( p, replace, replace_len * sizeof(WCHAR) );
    p += replace_len;
    name += prefix_len + match_len;
    len -= prefix_len + match_len;
    memcpy( p, name, len * sizeof(WCHAR) );
    p[len] = 0;
    attr->ObjectName = str;
    return TRUE;
}

/***********************************************************************
 *           init_redirects
 */
static void init_redirects(void)
{
    static const char system_dir[] = "/dosdevices/c:/windows/system32";
    char *dir;
    struct stat st;

    if (asprintf( &dir, "%s%s", config_dir, system_dir ) == -1) return;
    if (!stat( dir, &st ))
    {
        sysdir.dev = st.st_dev;
        sysdir.ino = st.st_ino;
    }
    *strrchr( dir, '/' ) = 0;
    if (!stat( dir, &st ))
    {
        windir.dev = st.st_dev;
        windir.ino = st.st_ino;
    }
    else ERR( "%s: %s\n", dir, strerror(errno) );
    free( dir );

}

/***********************************************************************
 *           get_redirect
 */
BOOL get_redirect( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *redir )
{
    const WCHAR *name = attr->ObjectName->Buffer;
    unsigned int i, prefix_len = 0, len = attr->ObjectName->Length / sizeof(WCHAR);

    redir->Buffer = NULL;
    if (!NtCurrentTeb64()) return FALSE;
    if (!len) return FALSE;

    if (!attr->RootDirectory)
    {
        prefix_len = wcslen( windirW );
        if (len < prefix_len || wcsnicmp( name, windirW, prefix_len )) return FALSE;
    }
    else
    {
        int fd, needs_close;
        struct stat st;

        if (server_get_unix_fd( attr->RootDirectory, 0, &fd, &needs_close, NULL, NULL )) return FALSE;
        fstat( fd, &st );
        if (needs_close) close( fd );
        if (!is_same_file( &windir, &st ))
        {
            if (!is_same_file( &sysdir, &st )) return FALSE;
            if (NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR]) return FALSE;
            if (name[0] == '\\') return FALSE;

            /* only check for paths that should NOT be redirected */
            for (i = 0; i < ARRAY_SIZE( no_redirect ); i++)
                if (starts_with_path( name, len, no_redirect[i] + 9 /* "system32\\" */)) return FALSE;

            /* redirect everything else */
            redir->Length = sizeof(syswow64dirW) + len * sizeof(WCHAR);
            redir->MaximumLength = redir->Length + sizeof(WCHAR);
            if (!(redir->Buffer = malloc( redir->MaximumLength ))) return FALSE;
            memcpy( redir->Buffer, syswow64dirW, sizeof(syswow64dirW) );
            memcpy( redir->Buffer + ARRAY_SIZE(syswow64dirW), name, len * sizeof(WCHAR) );
            redir->Buffer[redir->Length / sizeof(WCHAR)] = 0;
            attr->RootDirectory = 0;
            attr->ObjectName = redir;
            return TRUE;
        }
    }

    /* sysnative is redirected even when redirection is disabled */

    if (replace_path( attr, redir, prefix_len, sysnativeW, system32W )) return TRUE;

    if (NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR]) return FALSE;

    for (i = 0; i < ARRAY_SIZE( no_redirect ); i++)
        if (starts_with_path( name + prefix_len, len - prefix_len, no_redirect[i] )) return FALSE;

    if (replace_path( attr, redir, prefix_len, system32W, syswow64W )) return TRUE;
    if (replace_path( attr, redir, prefix_len, regeditW, syswow64_regeditW )) return TRUE;
    return FALSE;
}

#else  /* _WIN64 */

/* there are no redirects on 64-bit */
BOOL get_redirect( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *redir )
{
    redir->Buffer = NULL;
    return FALSE;
}

#endif


#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

/***********************************************************************
 *           init_files
 */
void init_files(void)
{
    HANDLE key;

#ifndef _WIN64
    if (is_old_wow64()) init_redirects();
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

    if (!open_hkcu_key( "Software\\Wine", &key ))
    {
        static WCHAR showdotfilesW[] = {'S','h','o','w','D','o','t','F','i','l','e','s',0};
        char tmp[80];
        DWORD dummy;
        UNICODE_STRING nameW;

        init_unicode_string( &nameW, showdotfilesW );
        if (!NtQueryValueKey( key, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            show_dot_files = IS_OPTION_TRUE( str[0] );
        }
        NtClose( key );
    }
}


/******************************************************************************
 *           get_dos_device
 *
 * Get the Unix path of a DOS device.
 */
static NTSTATUS get_dos_device( char **unix_name, int start_pos )
{
    struct stat st;
    char *new_name, *dev = *unix_name + start_pos;

    /* special case for drive devices */
    if (dev[0] && dev[1] == ':' && !dev[2]) strcpy( dev + 1, "::" );

    if (strchr( dev, '/' )) goto failed;

    for (;;)
    {
        if (!stat( *unix_name, &st ))
        {
            TRACE( "-> %s\n", debugstr_a(*unix_name));
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
            new_name = get_default_drive_device( *unix_name );
        }
        free( *unix_name );
        *unix_name = new_name;
        if (!new_name) return STATUS_BAD_DEVICE_TYPE;
        dev = NULL; /* last try */
    }
failed:
    free( *unix_name );
    *unix_name = NULL;
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
static NTSTATUS find_file_id( int root_fd, char **unix_name, ULONG *len, ULONGLONG file_id, dev_t dev, struct list *dir_queue )
{
    unsigned int pos;
    int dir_fd;
    DIR *dir;
    struct dirent *de;
    NTSTATUS status;
    struct stat st;
    char *name = *unix_name;

    while (!(status = next_dir_in_queue( dir_queue, name )))
    {
        if ((dir_fd = openat( root_fd, name, O_RDONLY )) == -1) continue;
        if (!(dir = fdopendir( dir_fd )))
        {
            close( dir_fd );
            continue;
        }
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
            if (fstatat( root_fd, name, &st, AT_SYMLINK_NOFOLLOW ) == -1) continue;
            if (st.st_dev != dev) continue;
            if (st.st_ino == file_id)
            {
                closedir( dir );
                return STATUS_SUCCESS;
            }
            if (!S_ISDIR( st.st_mode )) continue;
            if ((status = add_dir_to_queue( dir_queue, name )) != STATUS_SUCCESS)
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
static NTSTATUS file_id_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char **unix_name_ret,
                                           UNICODE_STRING *nt_name )
{
    enum server_fd_type type;
    int root_fd, needs_close;
    char *unix_name;
    ULONG len;
    NTSTATUS status;
    ULONGLONG file_id;
    struct stat st, root_st;
    struct list dir_queue = LIST_INIT( dir_queue );

    nt_name->Buffer = NULL;
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

    /* shortcut for ".." */
    if (!fstatat( root_fd, "..", &st, 0 ) && st.st_dev == root_st.st_dev && st.st_ino == file_id)
    {
        strcpy( unix_name, ".." );
        status = STATUS_SUCCESS;
    }
    else
    {
        status = add_dir_to_queue( &dir_queue, "." );
        if (!status)
            status = find_file_id( root_fd, &unix_name, &len, file_id, root_st.st_dev, &dir_queue );
        if (!status)  /* get rid of "./" prefix */
            memmove( unix_name, unix_name + 2, strlen(unix_name) - 1 );
        flush_dir_queue( &dir_queue );
    }

done:
    if (status == STATUS_SUCCESS)
    {
        TRACE( "%s -> %s\n", wine_dbgstr_longlong(file_id), debugstr_a(unix_name) );
        *unix_name_ret = unix_name;

        nt_name->MaximumLength = (strlen(unix_name) + 1) * sizeof(WCHAR);
        if ((nt_name->Buffer = malloc( nt_name->MaximumLength )))
        {
            DWORD i, len = ntdll_umbstowcs( unix_name, strlen(unix_name), nt_name->Buffer, strlen(unix_name) );
            nt_name->Buffer[len] = 0;
            nt_name->Length = len * sizeof(WCHAR);
            for (i = 0; i < len; i++) if (nt_name->Buffer[i] == '/') nt_name->Buffer[i] = '\\';
        }
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
static NTSTATUS lookup_unix_name( int root_fd, const WCHAR *name, int name_len, char **buffer, int unix_len,
                                  int pos, UINT disposition, BOOL is_unix )
{
    static const WCHAR invalid_charsW[] = { INVALID_NT_CHARS, '/', 0 };
    NTSTATUS status;
    int ret;
    struct stat st;
    char *unix_name = *buffer;
    const WCHAR *ptr, *end;

    /* check syntax of individual components */

    for (ptr = name, end = name + name_len; ptr < end; ptr++)
    {
        if (*ptr == '\\') return STATUS_OBJECT_NAME_INVALID;  /* duplicate backslash */
        if (*ptr == '.')
        {
            if (ptr + 1 == end) return STATUS_OBJECT_NAME_INVALID;  /* "." element */
            if (ptr[1] == '\\') return STATUS_OBJECT_NAME_INVALID;  /* "." element */
            if (ptr[1] == '.')
            {
                if (ptr + 2 == end) return STATUS_OBJECT_NAME_INVALID;  /* ".." element */
                if (ptr[2] == '\\') return STATUS_OBJECT_NAME_INVALID;  /* ".." element */
            }
        }
        /* check for invalid characters (all chars except 0 are valid for unix) */
        for ( ; ptr < end && *ptr != '\\'; ptr++)
        {
            if (!*ptr) return STATUS_OBJECT_NAME_INVALID;
            if (is_unix) continue;
            if (*ptr < 32 || wcschr( invalid_charsW, *ptr )) return STATUS_OBJECT_NAME_INVALID;
        }
    }

    /* try a shortcut first */

    unix_name[pos] = '/';
    ret = ntdll_wcstoumbs( name, name_len, unix_name + pos + 1, unix_len - pos - 1, TRUE );
    if (ret >= 0 && ret < unix_len - pos - 1)
    {
        char *p;
        unix_name[pos + 1 + ret] = 0;
        for (p = unix_name + pos ; *p; p++) if (*p == '\\') *p = '/';
        if (!fstatat( root_fd, unix_name, &st, 0 ))
        {
            if (disposition == FILE_CREATE) return STATUS_OBJECT_NAME_COLLISION;
            return STATUS_SUCCESS;
        }
    }

    if (!name_len)  /* empty name -> drive root doesn't exist */
        return STATUS_OBJECT_PATH_NOT_FOUND;
    if (is_unix && (disposition == FILE_OPEN || disposition == FILE_OVERWRITE))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    /* now do it component by component */

    while (name_len)
    {
        const WCHAR *end, *next;

        end = name;
        while (end < name + name_len && *end != '\\') end++;
        next = end;
        if (next < name + name_len) next++;
        name_len -= next - name;

        /* grow the buffer if needed */

        if (unix_len - pos < MAX_DIR_ENTRY_LEN + 3)
        {
            char *new_name;
            unix_len += 2 * MAX_DIR_ENTRY_LEN;
            if (!(new_name = realloc( unix_name, unix_len ))) return STATUS_NO_MEMORY;
            unix_name = *buffer = new_name;
        }

        status = find_file_in_dir( root_fd, unix_name, pos, name, end - name, is_unix );

        /* if this is the last element, not finding it is not necessarily fatal */
        if (!name_len)
        {
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                if (disposition != FILE_OPEN && disposition != FILE_OVERWRITE)
                {
                    ret = ntdll_wcstoumbs( name, end - name, unix_name + pos + 1, MAX_DIR_ENTRY_LEN + 1, TRUE );
                    if (ret > 0 && ret <= MAX_DIR_ENTRY_LEN)
                    {
                        unix_name[pos] = '/';
                        pos += ret + 1;
                        if (end < next) unix_name[pos++] = '/';
                        unix_name[pos] = 0;
                        status = STATUS_NO_SUCH_FILE;
                        break;
                    }
                }
            }
            else if (status == STATUS_SUCCESS && disposition == FILE_CREATE)
            {
                status = STATUS_OBJECT_NAME_COLLISION;
            }
            if (end < next) strcat( unix_name, "/" );
        }
        else if (status == STATUS_OBJECT_NAME_NOT_FOUND) status = STATUS_OBJECT_PATH_NOT_FOUND;

        if (status != STATUS_SUCCESS) break;

        pos += strlen( unix_name + pos );
        name = next;
    }

    return status;
}


/******************************************************************************
 *           nt_to_unix_file_name_no_root
 */
static NTSTATUS nt_to_unix_file_name_no_root( const UNICODE_STRING *nameW, char **unix_name_ret,
                                              UINT disposition )
{
    static const WCHAR unixW[] = {'u','n','i','x'};
    static const WCHAR invalid_charsW[] = { INVALID_NT_CHARS, 0 };

    NTSTATUS status = STATUS_SUCCESS;
    const WCHAR *name;
    struct stat st;
    char *unix_name;
    int pos, ret, name_len, unix_len, prefix_len;
    WCHAR prefix[MAX_DIR_ENTRY_LEN + 1];
    BOOLEAN is_unix = FALSE;

    name     = nameW->Buffer;
    name_len = nameW->Length / sizeof(WCHAR);

    if (!name_len || name[0] != '\\') return STATUS_OBJECT_PATH_SYNTAX_BAD;

    if (!(pos = get_dos_prefix_len( nameW )))
        return STATUS_BAD_DEVICE_TYPE;  /* no DOS prefix, assume NT native name */

    name += pos;
    name_len -= pos;

    if (!name_len) return STATUS_OBJECT_NAME_INVALID;

    /* check for sub-directory */
    for (pos = 0; pos < name_len && pos <= MAX_DIR_ENTRY_LEN; pos++)
    {
        if (name[pos] == '\\') break;
        if (name[pos] < 32 || wcschr( invalid_charsW, name[pos] ))
            return STATUS_OBJECT_NAME_INVALID;
        prefix[pos] = (name[pos] >= 'A' && name[pos] <= 'Z') ? name[pos] + 'a' - 'A' : name[pos];
    }
    if (pos > MAX_DIR_ENTRY_LEN) return STATUS_OBJECT_NAME_INVALID;

    if (pos >= 4 && !memcmp( prefix, unixW, sizeof(unixW) ))
    {
        /* allow slash for unix namespace */
        if (pos > 4 && prefix[4] == '/') pos = 4;
        is_unix = pos == 4;
    }
    prefix_len = pos;
    prefix[prefix_len] = 0;

    unix_len = name_len * 3 + MAX_DIR_ENTRY_LEN + 3;
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

    if (prefix_len == name_len)  /* no subdir, plain DOS device */
    {
        unix_name[pos + ret] = 0;
        *unix_name_ret = unix_name;
        return get_dos_device( unix_name_ret, pos );
    }
    pos += ret;

    /* check if prefix exists (except for DOS drives to avoid extra stat calls) */

    if (wcschr( prefix, '/' ))
    {
        free( unix_name );
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

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

    prefix_len++;  /* skip initial backslash */
    if (name_len > prefix_len && name[prefix_len] == '\\') prefix_len++;  /* allow a second backslash */
    name += prefix_len;
    name_len -= prefix_len;

    status = lookup_unix_name( AT_FDCWD, name, name_len, &unix_name, unix_len, pos, disposition, is_unix );
    if (status == STATUS_SUCCESS || status == STATUS_NO_SUCH_FILE)
    {
        TRACE( "%s -> %s\n", debugstr_us(nameW), debugstr_a(unix_name) );
        *unix_name_ret = unix_name;
    }
    else
    {
        TRACE( "%s not found in %s\n", debugstr_w(name), debugstr_an(unix_name, pos) );
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
NTSTATUS nt_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char **name_ret, UINT disposition )
{
    enum server_fd_type type;
    int root_fd, needs_close;
    const WCHAR *name;
    char *unix_name;
    int name_len, unix_len;
    NTSTATUS status;

    if (!attr->RootDirectory)  /* without root dir fall back to normal lookup */
        return nt_to_unix_file_name_no_root( attr->ObjectName, name_ret, disposition );

    name     = attr->ObjectName->Buffer;
    name_len = attr->ObjectName->Length / sizeof(WCHAR);

    if (name_len && name[0] == '\\') return STATUS_INVALID_PARAMETER;

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
            status = lookup_unix_name( root_fd, name, name_len, &unix_name, unix_len, 1, disposition, FALSE );
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
 *           wine_nt_to_unix_file_name
 *
 * Convert a file name from NT namespace to Unix namespace.
 *
 * If disposition is not FILE_OPEN or FILE_OVERWRITE, the last path
 * element doesn't have to exist; in that case STATUS_NO_SUCH_FILE is
 * returned, but the unix name is still filled in properly.
 */
NTSTATUS WINAPI wine_nt_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char *nameA, ULONG *size,
                                          UINT disposition )
{
    char *buffer = NULL;
    NTSTATUS status;
    UNICODE_STRING redir;
    OBJECT_ATTRIBUTES new_attr = *attr;

    get_redirect( &new_attr, &redir );
    status = nt_to_unix_file_name( &new_attr, &buffer, disposition );

    if (buffer)
    {
        struct stat st1, st2;
        char *name = buffer;

        /* remove dosdevices prefix for z: drive if it points to the Unix root */
        if (!strncmp( buffer, config_dir, strlen(config_dir) ) &&
            !strncmp( buffer + strlen(config_dir), "/dosdevices/z:/", 15 ))
        {
            char *p = buffer + strlen(config_dir) + 14;
            *p = 0;
            if (!stat( buffer, &st1 ) && !stat( "/", &st2 ) &&
                st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
                name = p;
            *p = '/';
        }

        if (*size > strlen(name)) strcpy( nameA, name );
        else status = STATUS_BUFFER_TOO_SMALL;
        *size = strlen(name) + 1;
        free( buffer );
    }
    free( redir.Buffer );
    return status;
}


/******************************************************************
 *		collapse_path
 *
 * Get rid of . and .. components in the path.
 */
static void collapse_path( WCHAR *path )
{
    WCHAR *p, *start, *next;

    /* convert every / into a \ */
    for (p = path; *p; p++) if (*p == '/') *p = '\\';

    p = path + 4;
    while (*p && *p != '\\') p++;
    start = p + 1;

    /* collapse duplicate backslashes */
    next = start;
    for (p = next; *p; p++) if (*p != '\\' || next[-1] != '\\') *next++ = *p;
    *next = 0;

    p = start;
    while (*p)
    {
        if (*p == '.')
        {
            switch(p[1])
            {
            case '\\': /* .\ component */
                next = p + 2;
                memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
                continue;
            case 0:  /* final . */
                if (p > start) p--;
                *p = 0;
                continue;
            case '.':
                if (p[2] == '\\')  /* ..\ component */
                {
                    next = p + 3;
                    if (p > start)
                    {
                        p--;
                        while (p > start && p[-1] != '\\') p--;
                    }
                    memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
                    continue;
                }
                else if (!p[2])  /* final .. */
                {
                    if (p > start)
                    {
                        p--;
                        while (p > start && p[-1] != '\\') p--;
                        if (p > start) p--;
                    }
                    *p = 0;
                    continue;
                }
                break;
            }
        }
        /* skip to the next component */
        while (*p && *p != '\\') p++;
        if (*p == '\\')
        {
            /* remove last dot in previous dir name */
            if (p > start && p[-1] == '.') memmove( p-1, p, (wcslen(p) + 1) * sizeof(WCHAR) );
            else p++;
        }
    }

    /* remove trailing spaces and dots (yes, Windows really does that, don't ask) */
    while (p > start && (p[-1] == ' ' || p[-1] == '.')) p--;
    *p = 0;
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
    WCHAR *buffer;
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
    collapse_path( buffer );
    *nt = buffer;
    return STATUS_SUCCESS;
}


/******************************************************************
 *           wine_unix_to_nt_file_name
 */
NTSTATUS WINAPI wine_unix_to_nt_file_name( const char *name, WCHAR *buffer, ULONG *size )
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
 *           get_full_path
 *
 * Simplified version of RtlGetFullPathName_U.
 */
NTSTATUS get_full_path( const WCHAR *name, const WCHAR *curdir, WCHAR **path )
{
    static const WCHAR uncW[] = {'\\','?','?','\\','U','N','C','\\',0};
    static const WCHAR devW[] = {'\\','?','?','\\',0};
    static const WCHAR unixW[] = {'u','n','i','x'};
    WCHAR *ret, root[] = {'\\','?','?','\\','C',':','\\',0};
    NTSTATUS status = STATUS_SUCCESS;
    const WCHAR *prefix;

    if (IS_SEPARATOR(name[0]) && IS_SEPARATOR(name[1]))  /* \\ prefix */
    {
        if ((name[2] == '.' || name[2] == '?') && IS_SEPARATOR(name[3])) /* \\?\ device */
        {
            name += 4;
            if (!wcsnicmp( name, unixW, 4 ) && IS_SEPARATOR(name[4]))  /* \\?\unix special name */
            {
                char *unix_name;
                name += 4;
                unix_name = malloc( wcslen(name) * 3 + 1 );
                ntdll_wcstoumbs( name, wcslen(name) + 1, unix_name, wcslen(name) * 3 + 1, FALSE );
                status = unix_to_nt_file_name( unix_name, path );
                free( unix_name );
                return status;
            }
            prefix = devW;
        }
        else prefix = uncW;  /* UNC path */
    }
    else if (IS_SEPARATOR(name[0]))  /* absolute path */
    {
        root[4] = curdir[4];
        prefix = root;
    }
    else if (name[0] && name[1] == ':')  /* drive letter */
    {
        root[4] = towupper(name[0]);
        name += 2;
        prefix = root;
    }
    else prefix = curdir;  /* relative path */

    ret = malloc( (wcslen(prefix) + wcslen(name) + 1) * sizeof(WCHAR) );
    wcscpy( ret, prefix );
    wcscat( ret, name );
    collapse_path( ret );
    *path = ret;
    return STATUS_SUCCESS;
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
                static char diskutil[] = "diskutil";
                static char unmount[] = "unmount";
                char *argv[4] = {diskutil, unmount, mount_point, NULL};
#else
                static char umount[] = "umount";
                char *argv[3] = {umount, mount_point, NULL};
#endif
                __wine_unix_spawnvp( argv, TRUE );
#ifdef linux
                /* umount will fail to release the loop device since we still have
                    a handle to it, so we release it here */
                if (major(st.st_rdev) == LOOP_MAJOR) ioctl( unix_fd, 0x4c01 /*LOOP_CLR_FD*/, 0 );
#endif
                /* Add in a small delay. Without this subsequent tasks
                    like IOCTL_STORAGE_EJECT_MEDIA might fail. */
                usleep( 100000 );
            }
        }
        if (needs_close) close( unix_fd );
    }
    return status;
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
    struct object_attributes *objattr;
    unsigned int status;
    data_size_t len;

    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

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
    OBJECT_ATTRIBUTES new_attr;
    UNICODE_STRING nt_name;
    char *unix_name;
    BOOL name_hidden = FALSE;
    BOOL created = FALSE;
    unsigned int status;

    TRACE( "handle=%p access=%08x name=%s objattr=%08x root=%p sec=%p io=%p alloc_size=%p "
           "attr=%08x sharing=%08x disp=%d options=%08x ea=%p.0x%08x\n",
           handle, access, debugstr_us(attr->ObjectName), attr->Attributes,
           attr->RootDirectory, attr->SecurityDescriptor, io, alloc_size,
           attributes, sharing, disposition, options, ea_buffer, ea_length );

    *handle = 0;
    if (!attr || !attr->ObjectName) return STATUS_INVALID_PARAMETER;

    if (alloc_size) FIXME( "alloc_size not supported\n" );

    new_attr = *attr;
    if (options & FILE_OPEN_BY_FILE_ID)
    {
        status = file_id_to_unix_file_name( &new_attr, &unix_name, &nt_name );
        if (!status) new_attr.ObjectName = &nt_name;
    }
    else
    {
        get_redirect( &new_attr, &nt_name );
        status = nt_to_unix_file_name( &new_attr, &unix_name, disposition );
    }

    if (status == STATUS_BAD_DEVICE_TYPE)
    {
        status = server_open_file_object( handle, access, &new_attr, sharing, options );
        if (status == STATUS_SUCCESS) io->Information = FILE_OPENED;
        free( nt_name.Buffer );
        return io->Status = status;
    }

    if (status == STATUS_NO_SUCH_FILE && disposition != FILE_OPEN && disposition != FILE_OVERWRITE)
    {
        created = TRUE;
        status = STATUS_SUCCESS;
    }

    if (status == STATUS_SUCCESS)
    {
        name_hidden = is_hidden_file( unix_name );
        status = open_unix_file( handle, unix_name, access, &new_attr, attributes,
                                 sharing, disposition, options, ea_buffer, ea_length );
        free( unix_name );
    }
    else WARN( "%s not found (%x)\n", debugstr_us(attr->ObjectName), status );

    if (status == STATUS_SUCCESS)
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

        if (io->Information == FILE_CREATED &&
            ((attributes & XATTR_ATTRIBS_MASK) || name_hidden))
        {
            int fd, needs_close;

            /* set any DOS extended attributes */
            if (!server_get_unix_fd( *handle, 0, &fd, &needs_close, NULL, NULL ))
            {
                if (fd_set_dos_attrib( fd, attributes, TRUE ) == -1 && errno != ENOTSUP)
                    WARN( "Failed to set extended attribute " SAMBA_XATTR_DOS_ATTRIB ". errno %d (%s)",
                          errno, strerror( errno ) );
                if (needs_close) close( fd );
            }
        }
    }
    else if (status == STATUS_TOO_MANY_OPENED_FILES)
    {
        static int once;
        if (!once++) ERR_(winediag)( "Too many open files, ulimit -n probably needs to be increased\n" );
    }

    free( nt_name.Buffer );
    return io->Status = status;
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
    unsigned int status;
    data_size_t len;
    struct object_attributes *objattr;

    TRACE( "%p %08x %p %p %08x %08x %08x %p\n",
           handle, access, attr, io, options, quota, msg_size, timeout );

    *handle = 0;
    if (!attr) return STATUS_INVALID_PARAMETER;
    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

    SERVER_START_REQ( create_mailslot )
    {
        req->access       = access;
        req->options      = options;
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
    unsigned int status;
    data_size_t len;
    struct object_attributes *objattr;

    *handle = 0;
    if (!attr) return STATUS_INVALID_PARAMETER;

    TRACE( "(%p %x %s %p %x %d %x %d %d %d %d %d %d %p)\n",
           handle, access, debugstr_us(attr->ObjectName), io, sharing, dispo,
           options, pipe_type, read_mode, completion_mode, max_inst,
           inbound_quota, outbound_quota, timeout );

    /* assume we only get relative timeout */
    if (timeout && timeout->QuadPart > 0) FIXME( "Wrong time %s\n", wine_dbgstr_longlong(timeout->QuadPart) );

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
        req->disposition  = dispo;
        req->maxinstances = max_inst;
        req->outsize = outbound_quota;
        req->insize  = inbound_quota;
        req->timeout = timeout ? timeout->QuadPart : 0ULL;
        wine_server_add_data( req, objattr, len );
        if (!(status = wine_server_call( req )))
        {
            *handle = wine_server_ptr_handle( reply->handle );
            io->Information = reply->created ? FILE_CREATED : FILE_OPENED;
        }
    }
    SERVER_END_REQ;

    free( objattr );
    return io->Status = status;
}


/******************************************************************
 *              NtDeleteFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtDeleteFile( OBJECT_ATTRIBUTES *attr )
{
    HANDLE handle;
    NTSTATUS status;
    char *unix_name;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES new_attr = *attr;

    get_redirect( &new_attr, &nt_name );
    if (!(status = nt_to_unix_file_name( &new_attr, &unix_name, FILE_OPEN )))
    {
        if (!(status = open_unix_file( &handle, unix_name, GENERIC_READ | GENERIC_WRITE | DELETE, &new_attr,
                                       0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
                                       FILE_DELETE_ON_CLOSE, NULL, 0 )))
            NtClose( handle );
        free( unix_name );
    }
    free( nt_name.Buffer );
    return status;
}


/******************************************************************************
 *              NtQueryFullAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryFullAttributesFile( const OBJECT_ATTRIBUTES *attr,
                                           FILE_NETWORK_OPEN_INFORMATION *info )
{
    char *unix_name;
    unsigned int status;
    UNICODE_STRING redir;
    OBJECT_ATTRIBUTES new_attr = *attr;

    get_redirect( &new_attr, &redir );
    if (!(status = nt_to_unix_file_name( &new_attr, &unix_name, FILE_OPEN )))
    {
        ULONG attributes;
        struct stat st;

        if (get_file_info( unix_name, &st, &attributes ) == -1)
            status = errno_to_status( errno );
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
            fill_file_info( &st, attributes, info, FileNetworkOpenInformation );
        free( unix_name );
    }
    else WARN( "%s not found (%x)\n", debugstr_us(attr->ObjectName), status );
    free( redir.Buffer );
    return status;
}


/******************************************************************************
 *              NtQueryAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryAttributesFile( const OBJECT_ATTRIBUTES *attr, FILE_BASIC_INFORMATION *info )
{
    char *unix_name;
    unsigned int status;
    UNICODE_STRING redir;
    OBJECT_ATTRIBUTES new_attr = *attr;

    get_redirect( &new_attr, &redir );
    if (!(status = nt_to_unix_file_name( &new_attr, &unix_name, FILE_OPEN )))
    {
        ULONG attributes;
        struct stat st;

        if (get_file_info( unix_name, &st, &attributes ) == -1)
            status = errno_to_status( errno );
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
            status = fill_file_info( &st, attributes, info, FileBasicInformation );
        free( unix_name );
    }
    else WARN( "%s not found (%x)\n", debugstr_us(attr->ObjectName), status );
    free( redir.Buffer );
    return status;
}


/******************************************************************************
 *              NtQueryInformationFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                        void *ptr, ULONG len, FILE_INFORMATION_CLASS class )
{
    static const size_t info_sizes[FileMaximumInformation] =
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
        0,                                             /* FileDispositionInformationEx */
        0,                                             /* FileRenameInformationEx */
        0,                                             /* FileRenameInformationExBypassAccessCheck */
        0,                                             /* FileDesiredStorageClassInformation */
        sizeof(FILE_STAT_INFORMATION),                 /* FileStatInformation */
        0,                                             /* FileMemoryPartitionInformation */
        0,                                             /* FileStatLxInformation */
        0,                                             /* FileCaseSensitiveInformation */
        0,                                             /* FileLinkInformationEx */
        0,                                             /* FileLinkInformationExBypassAccessCheck */
        0,                                             /* FileStorageReserveIdInformation */
        0,                                             /* FileCaseSensitiveInformationForceAccessCheck */
        0,                                             /* FileKnownFolderInformation */
    };

    struct stat st;
    int fd, needs_close = FALSE;
    ULONG attr;
    unsigned int options;
    unsigned int status;

    TRACE( "(%p,%p,%p,0x%08x,0x%08x)\n", handle, io, ptr, len, class);

    io->Information = 0;

    if (class <= 0 || class >= FileMaximumInformation)
        return io->Status = STATUS_INVALID_INFO_CLASS;
    if (!info_sizes[class])
        return server_get_file_info( handle, io, ptr, len, class );
    if (len < info_sizes[class])
        return io->Status = STATUS_INFO_LENGTH_MISMATCH;

    if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, &options )))
    {
        if (status != STATUS_BAD_DEVICE_TYPE) return io->Status = status;
        return server_get_file_info( handle, io, ptr, len, class );
    }

    switch (class)
    {
    case FileBasicInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1)
            status = errno_to_status( errno );
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
            fill_file_info( &st, attr, ptr, class );
        break;
    case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION *info = ptr;

            if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
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
            if (res == (off_t)-1) status = errno_to_status( errno );
            else info->CurrentByteOffset.QuadPart = res;
        }
        break;
    case FileInternalInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
        else fill_file_info( &st, attr, ptr, class );
        break;
    case FileEaInformation:
        {
            FILE_EA_INFORMATION *info = ptr;
            info->EaSize = 0;
        }
        break;
    case FileEndOfFileInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
        else fill_file_info( &st, attr, ptr, class );
        break;
    case FileAllInformation:
        {
            FILE_ALL_INFORMATION *info = ptr;

            if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
            else
            {
                LONG name_len = len - FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName);

                fill_file_info( &st, attr, info, FileAllInformation );
                info->StandardInformation.DeletePending = FALSE; /* FIXME */
                info->EaInformation.EaSize = 0;
                info->AccessInformation.AccessFlags = 0;  /* FIXME */
                info->PositionInformation.CurrentByteOffset.QuadPart = lseek( fd, 0, SEEK_CUR );
                info->ModeInformation.Mode = 0;  /* FIXME */
                info->AlignmentInformation.AlignmentRequirement = 1;  /* FIXME */
                status = server_get_name_info( handle, &info->NameInformation, &name_len );
                io->Information = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + name_len;
            }
        }
        break;
    case FileNameInformation:
        {
            LONG name_len = len - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
            status = server_get_name_info( handle, ptr, &name_len );
            io->Information = offsetof( FILE_NAME_INFORMATION, FileName ) + name_len;
        }
        break;
    case FileNetworkOpenInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
        else fill_file_info( &st, attr, ptr, FileNetworkOpenInformation );
        break;
    case FileIdInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
        else
        {
            struct mountmgr_unix_drive drive;
            FILE_ID_INFORMATION *info = ptr;

            info->VolumeSerialNumber = 0;
            if (!get_mountmgr_fs_info( handle, fd, &drive, sizeof(drive) ))
                info->VolumeSerialNumber = drive.serial;
            memset( &info->FileId, 0, sizeof(info->FileId) );
            *(ULONGLONG *)&info->FileId = st.st_ino;
        }
        break;
    case FileAttributeTagInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
        else
        {
            FILE_ATTRIBUTE_TAG_INFORMATION *info = ptr;
            info->FileAttributes = attr;
            info->ReparseTag = 0; /* FIXME */
            if ((options & FILE_OPEN_REPARSE_POINT) && fd_is_mount_point( fd, &st ))
                info->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
        }
        break;
    case FileStatInformation:
        if (fd_get_file_info( fd, options, &st, &attr ) == -1) status = errno_to_status( errno );
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
        {
            FILE_STAT_INFORMATION *info = ptr;
            FILE_BASIC_INFORMATION basic;
            FILE_STANDARD_INFORMATION std;

            fill_file_info( &st, attr, &basic, FileBasicInformation );
            fill_file_info( &st, attr, &std, FileStandardInformation );

            info->FileId.QuadPart = st.st_ino;
            info->CreationTime   = basic.CreationTime;
            info->LastAccessTime = basic.LastAccessTime;
            info->LastWriteTime  = basic.LastWriteTime;
            info->ChangeTime     = basic.ChangeTime;
            info->AllocationSize = std.AllocationSize;
            info->EndOfFile      = std.EndOfFile;
            info->FileAttributes = attr;
            info->ReparseTag     = 0; /* FIXME */
            if ((options & FILE_OPEN_REPARSE_POINT) && fd_is_mount_point( fd, &st ))
                info->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
            info->NumberOfLinks  = std.NumberOfLinks;
            info->EffectiveAccess = FILE_ALL_ACCESS; /* FIXME */
        }
        break;
    default:
        FIXME("Unsupported class (%d)\n", class);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    if (needs_close) close( fd );
    if (status == STATUS_SUCCESS && !io->Information) io->Information = info_sizes[class];
    return io->Status = status;
}


/******************************************************************************
 *              NtSetInformationFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                      void *ptr, ULONG len, FILE_INFORMATION_CLASS class )
{
    int fd, needs_close;
    unsigned int status = STATUS_SUCCESS;

    TRACE( "(%p,%p,%p,0x%08x,0x%08x)\n", handle, io, ptr, len, class );

    switch (class)
    {
    case FileBasicInformation:
        if (len >= sizeof(FILE_BASIC_INFORMATION))
        {
            const FILE_BASIC_INFORMATION *info = ptr;
            LARGE_INTEGER mtime, atime;
            char *unix_name;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return io->Status = status;

            if (server_get_unix_name( handle, &unix_name )) unix_name = NULL;

            mtime.QuadPart = info->LastWriteTime.QuadPart == -1 ? 0 : info->LastWriteTime.QuadPart;
            atime.QuadPart = info->LastAccessTime.QuadPart == -1 ? 0 : info->LastAccessTime.QuadPart;

            if (atime.QuadPart || mtime.QuadPart)
                status = set_file_times( fd, &mtime, &atime );

            if (status == STATUS_SUCCESS)
                status = fd_set_file_info( fd, info->FileAttributes,
                                           unix_name && is_hidden_file( unix_name ));

            if (needs_close) close( fd );
            free( unix_name );
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FilePositionInformation:
        if (len >= sizeof(FILE_POSITION_INFORMATION))
        {
            const FILE_POSITION_INFORMATION *info = ptr;

            if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return io->Status = status;

            if (lseek( fd, info->CurrentByteOffset.QuadPart, SEEK_SET ) == (off_t)-1)
                status = errno_to_status( errno );

            if (needs_close) close( fd );
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileEndOfFileInformation:
        if (len >= sizeof(FILE_END_OF_FILE_INFORMATION))
        {
            const FILE_END_OF_FILE_INFORMATION *info = ptr;

            SERVER_START_REQ( set_fd_eof_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->eof      = info->EndOfFile.QuadPart;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FilePipeInformation:
        if (len >= sizeof(FILE_PIPE_INFORMATION))
        {
            FILE_PIPE_INFORMATION *info = ptr;

            if ((info->CompletionMode | info->ReadMode) & ~1)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            SERVER_START_REQ( set_named_pipe_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags  = (info->CompletionMode ? NAMED_PIPE_NONBLOCKING_MODE    : 0) |
                              (info->ReadMode       ? NAMED_PIPE_MESSAGE_STREAM_READ : 0);
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileMailslotSetInformation:
        {
            FILE_MAILSLOT_SET_INFORMATION *info = ptr;

            SERVER_START_REQ( set_mailslot_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags = MAILSLOT_SET_READ_TIMEOUT;
                req->read_timeout = info->ReadTimeout.QuadPart;
                status = wine_server_call( req );
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
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        else status = STATUS_INVALID_PARAMETER_3;
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
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        else status = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case FileIoPriorityHintInformation:
        if (len >= sizeof(FILE_IO_PRIORITY_HINT_INFO))
        {
            FILE_IO_PRIORITY_HINT_INFO *info = ptr;
            if (info->PriorityHint < MaximumIoPriorityHintType)
                TRACE( "ignoring FileIoPriorityHintInformation %u\n", info->PriorityHint );
            else
                status = STATUS_INVALID_PARAMETER;
        }
        else status = STATUS_INFO_LENGTH_MISMATCH;
        break;

    case FileAllInformation:
        status = STATUS_INVALID_INFO_CLASS;
        break;

    case FileValidDataLengthInformation:
        if (len >= sizeof(FILE_VALID_DATA_LENGTH_INFORMATION))
        {
            struct stat st;
            const FILE_VALID_DATA_LENGTH_INFORMATION *info = ptr;

            if ((status = server_get_unix_fd( handle, FILE_WRITE_DATA, &fd, &needs_close, NULL, NULL )))
                return io->Status = status;

            if (fstat( fd, &st ) == -1) status = errno_to_status( errno );
            else if (info->ValidDataLength.QuadPart <= 0 || (off_t)info->ValidDataLength.QuadPart > st.st_size)
                status = STATUS_INVALID_PARAMETER;
            else
            {
#ifdef HAVE_POSIX_FALLOCATE
                int err;
                if ((err = posix_fallocate( fd, 0, (off_t)info->ValidDataLength.QuadPart )) != 0)
                {
                    if (err == EOPNOTSUPP) WARN( "posix_fallocate not supported on this filesystem\n" );
                    else status = errno_to_status( err );
                }
#else
                WARN( "setting valid data length not supported\n" );
#endif
            }
            if (needs_close) close( fd );
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileDispositionInformation:
        if (len >= sizeof(FILE_DISPOSITION_INFORMATION))
        {
            FILE_DISPOSITION_INFORMATION *info = ptr;

            SERVER_START_REQ( set_fd_disp_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->flags    = info->DoDeleteFile ? FILE_DISPOSITION_DELETE : FILE_DISPOSITION_DO_NOT_DELETE;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileDispositionInformationEx:
        if (len >= sizeof(FILE_DISPOSITION_INFORMATION_EX))
        {
            FILE_DISPOSITION_INFORMATION_EX *info = ptr;

            if (info->Flags & FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK)
                FIXME( "FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK not supported\n" );

            SERVER_START_REQ( set_fd_disp_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->flags    = info->Flags;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileRenameInformation:
    case FileRenameInformationEx:
        if (len >= sizeof(FILE_RENAME_INFORMATION))
        {
            FILE_RENAME_INFORMATION *info = ptr;
            unsigned int flags;
            UNICODE_STRING name_str, redir;
            OBJECT_ATTRIBUTES attr;
            char *unix_name;

            if (class == FileRenameInformation)
                flags = info->ReplaceIfExists ? FILE_RENAME_REPLACE_IF_EXISTS : 0;
            else
                flags = info->Flags;

            if (flags & ~(FILE_RENAME_REPLACE_IF_EXISTS | FILE_RENAME_IGNORE_READONLY_ATTRIBUTE))
                FIXME( "unsupported flags: %#x\n", flags );

            name_str.Buffer = info->FileName;
            name_str.Length = info->FileNameLength;
            name_str.MaximumLength = info->FileNameLength + sizeof(WCHAR);
            InitializeObjectAttributes( &attr, &name_str, OBJ_CASE_INSENSITIVE, info->RootDirectory, NULL );
            get_redirect( &attr, &redir );

            status = nt_to_unix_file_name( &attr, &unix_name, FILE_OPEN_IF );
            if (status == STATUS_SUCCESS || status == STATUS_NO_SUCH_FILE)
            {
                SERVER_START_REQ( set_fd_name_info )
                {
                    req->handle   = wine_server_obj_handle( handle );
                    req->rootdir  = wine_server_obj_handle( attr.RootDirectory );
                    req->namelen  = attr.ObjectName->Length;
                    req->link     = FALSE;
                    req->flags    = flags;
                    wine_server_add_data( req, attr.ObjectName->Buffer, attr.ObjectName->Length );
                    wine_server_add_data( req, unix_name, strlen(unix_name) );
                    status = wine_server_call( req );
                }
                SERVER_END_REQ;

                free( unix_name );
            }
            free( redir.Buffer );
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileLinkInformation:
    case FileLinkInformationEx:
        if (len >= sizeof(FILE_LINK_INFORMATION))
        {
            FILE_LINK_INFORMATION *info = ptr;
            unsigned int flags;
            UNICODE_STRING name_str, redir;
            OBJECT_ATTRIBUTES attr;
            char *unix_name;

            if (class == FileLinkInformation)
                flags = info->ReplaceIfExists ? FILE_LINK_REPLACE_IF_EXISTS : 0;
            else
                flags = info->Flags;

            if (flags & ~(FILE_LINK_REPLACE_IF_EXISTS | FILE_LINK_IGNORE_READONLY_ATTRIBUTE))
                FIXME( "unsupported flags: %#x\n", flags );

            name_str.Buffer = info->FileName;
            name_str.Length = info->FileNameLength;
            name_str.MaximumLength = info->FileNameLength + sizeof(WCHAR);
            InitializeObjectAttributes( &attr, &name_str, OBJ_CASE_INSENSITIVE, info->RootDirectory, NULL );
            get_redirect( &attr, &redir );

            status = nt_to_unix_file_name( &attr, &unix_name, FILE_OPEN_IF );
            if (status == STATUS_SUCCESS || status == STATUS_NO_SUCH_FILE)
            {
                SERVER_START_REQ( set_fd_name_info )
                {
                    req->handle   = wine_server_obj_handle( handle );
                    req->rootdir  = wine_server_obj_handle( attr.RootDirectory );
                    req->namelen  = attr.ObjectName->Length;
                    req->link     = TRUE;
                    req->flags    = flags;
                    wine_server_add_data( req, attr.ObjectName->Buffer, attr.ObjectName->Length );
                    wine_server_add_data( req, unix_name, strlen(unix_name) );
                    status  = wine_server_call( req );
                }
                SERVER_END_REQ;

                free( unix_name );
            }
            free( redir.Buffer );
        }
        else status = STATUS_INVALID_PARAMETER_3;
        break;

    default:
        FIXME("Unsupported class (%d)\n", class);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    io->Information = 0;
    return io->Status = status;
}


/***********************************************************************
 *                  Asynchronous file I/O                              *
 */

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

void release_fileio( struct async_fileio *io )
{
    for (;;)
    {
        struct async_fileio *next = fileio_freelist;
        io->next = next;
        if (InterlockedCompareExchangePointer( (void **)&fileio_freelist, io, next ) == next) return;
    }
}

struct async_fileio *alloc_fileio( DWORD size, async_callback_t callback, HANDLE handle )
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

/* callback for irp async I/O completion */
static BOOL irp_completion( void *user, ULONG_PTR *info, unsigned int *status )
{
    struct async_irp *async = user;

    if (*status == STATUS_ALERTED)
    {
        SERVER_START_REQ( get_async_result )
        {
            req->user_arg = wine_server_client_ptr( async );
            wine_server_set_reply( req, async->buffer, async->size );
            *status = virtual_locked_server_call( req );
        }
        SERVER_END_REQ;
    }
    release_fileio( &async->io );
    return TRUE;
}

static BOOL async_read_proc( void *user, ULONG_PTR *info, unsigned int *status )
{
    struct async_fileio_read *fileio = user;
    int fd, needs_close, result;

    switch (*status)
    {
    case STATUS_ALERTED: /* got some new data */
        /* check to see if the data is ready (non-blocking) */
        if ((*status = server_get_unix_fd( fileio->io.handle, FILE_READ_DATA, &fd,
                                          &needs_close, NULL, NULL )))
            break;

        result = virtual_locked_read(fd, &fileio->buffer[fileio->already], fileio->count-fileio->already);
        if (needs_close) close( fd );

        if (result < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                return FALSE;

            /* check to see if the transfer is complete */
            *status = errno_to_status( errno );
        }
        else if (result == 0)
        {
            *status = fileio->already ? STATUS_SUCCESS : STATUS_PIPE_BROKEN;
        }
        else
        {
            fileio->already += result;

            if (fileio->already < fileio->count && !fileio->avail_mode)
                return FALSE;

            *status = STATUS_SUCCESS;
        }
        break;

    case STATUS_TIMEOUT:
    case STATUS_IO_TIMEOUT:
        if (fileio->already) *status = STATUS_SUCCESS;
        break;
    }

    *info = fileio->already;
    release_fileio( &fileio->io );
    return TRUE;
}

static BOOL async_write_proc( void *user, ULONG_PTR *info, unsigned int *status )
{
    struct async_fileio_write *fileio = user;
    int result, fd, needs_close;
    enum server_fd_type type;

    switch (*status)
    {
    case STATUS_ALERTED:
        /* write some data (non-blocking) */
        if ((*status = server_get_unix_fd( fileio->io.handle, FILE_WRITE_DATA, &fd,
                                          &needs_close, &type, NULL )))
            break;

        result = write( fd, &fileio->buffer[fileio->already], fileio->count - fileio->already );

        if (needs_close) close( fd );

        if (result < 0)
        {
            if (errno == EAGAIN || errno == EINTR) return FALSE;
            *status = errno_to_status( errno );
        }
        else
        {
            fileio->already += result;
            if (fileio->already < fileio->count) return FALSE;
            *status = STATUS_SUCCESS;
        }
        break;

    case STATUS_TIMEOUT:
    case STATUS_IO_TIMEOUT:
        if (fileio->already) *status = STATUS_SUCCESS;
        break;
    }

    *info = fileio->already;
    release_fileio( &fileio->io );
    return TRUE;
}

static void set_sync_iosb( IO_STATUS_BLOCK *io, NTSTATUS status, ULONG_PTR info, unsigned int options )
{
    if (in_wow64_call() && !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)))
    {
        IO_STATUS_BLOCK32 *io32 = io->Pointer;

        io32->Status = status;
        io32->Information = info;
    }
    else
    {
        io->Status = status;
        io->Information = info;
    }
}

/* do a read call through the server */
static unsigned int server_read_file( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                      IO_STATUS_BLOCK *io, void *buffer, ULONG size,
                                      LARGE_INTEGER *offset, ULONG *key )
{
    struct async_irp *async;
    unsigned int status;
    HANDLE wait_handle;
    ULONG options;

    if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
        return STATUS_NO_MEMORY;

    async->buffer  = buffer;
    async->size    = size;

    SERVER_START_REQ( read )
    {
        req->async = server_async( handle, &async->io, event, apc, apc_context, iosb_client_ptr(io) );
        req->pos   = offset ? offset->QuadPart : 0;
        wine_server_set_reply( req, buffer, size );
        status = virtual_locked_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if (wait_handle && status != STATUS_PENDING)
            set_sync_iosb( io, status, wine_server_reply_size( reply ), options );
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) free( async );

    if (wait_handle) status = wait_async( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT) );
    return status;
}

/* do a write call through the server */
static unsigned int server_write_file( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                       IO_STATUS_BLOCK *io, const void *buffer, ULONG size,
                                       LARGE_INTEGER *offset, ULONG *key )
{
    struct async_irp *async;
    unsigned int status;
    HANDLE wait_handle;
    ULONG options;

    if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
        return STATUS_NO_MEMORY;

    async->buffer  = NULL;
    async->size    = 0;

    SERVER_START_REQ( write )
    {
        req->async = server_async( handle, &async->io, event, apc, apc_context, iosb_client_ptr(io) );
        req->pos   = offset ? offset->QuadPart : 0;
        wine_server_add_data( req, buffer, size );
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if (wait_handle && status != STATUS_PENDING)
            set_sync_iosb( io, status, reply->size, options );
    }
    SERVER_END_REQ;

    if (status != STATUS_PENDING) free( async );

    if (wait_handle) status = wait_async( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT) );
    return status;
}

/* do an ioctl call through the server */
static NTSTATUS server_ioctl_file( HANDLE handle, HANDLE event,
                                   PIO_APC_ROUTINE apc, PVOID apc_context,
                                   IO_STATUS_BLOCK *io, UINT code,
                                   const void *in_buffer, UINT in_size,
                                   PVOID out_buffer, UINT out_size )
{
    struct async_irp *async;
    unsigned int status;
    HANDLE wait_handle;
    ULONG options;

    if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
        return STATUS_NO_MEMORY;
    async->buffer  = out_buffer;
    async->size    = out_size;

    SERVER_START_REQ( ioctl )
    {
        req->code        = code;
        req->async       = server_async( handle, &async->io, event, apc, apc_context, iosb_client_ptr(io) );
        wine_server_add_data( req, in_buffer, in_size );
        if ((code & 3) != METHOD_BUFFERED) wine_server_add_data( req, out_buffer, out_size );
        wine_server_set_reply( req, out_buffer, out_size );
        status = virtual_locked_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if (wait_handle && status != STATUS_PENDING)
            set_sync_iosb( io, status, wine_server_reply_size( reply ), options );
    }
    SERVER_END_REQ;

    if (status == STATUS_NOT_SUPPORTED)
        WARN("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
             code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);

    if (status != STATUS_PENDING) free( async );

    if (wait_handle) status = wait_async( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT) );
    return status;
}


struct io_timeouts
{
    int interval;   /* max interval between two bytes */
    int total;      /* total timeout for the whole operation */
    int end_time;   /* absolute time of end of operation */
};

/* retrieve the I/O timeouts to use for a given handle */
static unsigned int get_io_timeouts( HANDLE handle, enum server_fd_type type, ULONG count, BOOL is_read,
                                     struct io_timeouts *timeouts )
{
    timeouts->interval = timeouts->total = -1;

    switch(type)
    {
    case FD_TYPE_SERIAL:
    {
        /* GetCommTimeouts */
        SERIAL_TIMEOUTS st;

        if (sync_ioctl( handle, IOCTL_SERIAL_GET_TIMEOUTS, NULL, 0, &st, sizeof(st) ))
            break;

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

        if (!(status = sync_ioctl( handle, IOCTL_SERIAL_GET_TIMEOUTS, NULL, 0, &st, sizeof(st) )))
        {
            *avail_mode = (!st.ReadTotalTimeoutMultiplier &&
                           !st.ReadTotalTimeoutConstant &&
                           st.ReadIntervalTimeout == MAXDWORD);
        }
        break;
    }
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
static unsigned int register_async_file_read( HANDLE handle, HANDLE event,
                                              PIO_APC_ROUTINE apc, void *apc_user,
                                              client_ptr_t iosb, void *buffer,
                                              ULONG already, ULONG length, BOOL avail_mode )
{
    struct async_fileio_read *fileio;
    unsigned int status;

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

/* notify direct completion of async and close the wait handle if it is no longer needed */
void set_async_direct_result( HANDLE *async_handle, unsigned int options, IO_STATUS_BLOCK *io,
                              NTSTATUS status, ULONG_PTR information, BOOL mark_pending )
{
    unsigned int ret;

    /* if we got STATUS_ALERTED, we must have a valid async handle */
    assert( *async_handle );

    if (!NT_ERROR(status) && status != STATUS_PENDING)
        set_sync_iosb( io, status, information, options );

    SERVER_START_REQ( set_async_direct_result )
    {
        req->handle       = wine_server_obj_handle( *async_handle );
        req->status       = status;
        req->information  = information;
        req->mark_pending = mark_pending;
        ret = wine_server_call( req );
        if (ret == STATUS_SUCCESS)
            *async_handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    if (ret != STATUS_SUCCESS)
        ERR( "cannot report I/O result back to server: %#x\n", ret );
}

/* complete async file I/O, signaling completion in all ways necessary */
void file_complete_async( HANDLE handle, unsigned int options, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                          IO_STATUS_BLOCK *io, NTSTATUS status, ULONG_PTR information )
{
    ULONG_PTR iosb_ptr = iosb_client_ptr(io);

    set_sync_iosb( io, status, information, options );
    if (event) NtSetEvent( event, NULL );
    if (apc)
        NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc, (ULONG_PTR)apc_user, iosb_ptr, 0 );
    else if (apc_user && !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)))
        add_completion( handle, (ULONG_PTR)apc_user, status, information, FALSE );
}


static unsigned int set_pending_write( HANDLE device )
{
    unsigned int status;

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
    unsigned int status, ret_status;
    UINT total = 0;
    client_ptr_t iosb_ptr = iosb_client_ptr(io);
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
    else if (type == FD_TYPE_SOCKET)
    {
        status = sock_read( handle, unix_handle, event, apc, apc_user, io, buffer, length );
        if (needs_close) close( unix_handle );
        return status;
    }

    if (type == FD_TYPE_SERIAL && async_read && length)
    {
        /* an asynchronous serial port read with a read interval timeout needs to
           skip the synchronous read to make sure that the server starts the read
           interval timer after the first read */
        if ((status = get_io_timeouts( handle, type, length, TRUE, &timeouts ))) goto err;
        if (timeouts.interval)
        {
            status = register_async_file_read( handle, event, apc, apc_user, iosb_ptr,
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
            status = register_async_file_read( handle, event, apc, apc_user, iosb_ptr,
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
                    status = STATUS_TIMEOUT;
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
        set_sync_iosb( io, status, total, options );
        TRACE("= SUCCESS (%u)\n", total);
        if (event) NtSetEvent( event, NULL );
        if (apc && (!status || async_read)) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                                              (ULONG_PTR)apc_user, iosb_ptr, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }

    ret_status = async_read && type == FD_TYPE_FILE && (status == STATUS_SUCCESS || status == STATUS_END_OF_FILE)
            ? STATUS_PENDING : status;

    if (send_completion && async_read)
        add_completion( handle, cvalue, status, total, ret_status == STATUS_PENDING );
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
    unsigned int options, status;
    UINT pos = 0, total = 0;
    client_ptr_t iosb_ptr = iosb_client_ptr(io);
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
    set_sync_iosb( io, status, total, options );
    TRACE("= 0x%08x (%u)\n", status, total);
    if (event) NtSetEvent( event, NULL );
    if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc, (ULONG_PTR)apc_user, iosb_ptr, 0 );
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
    unsigned int status, ret_status;
    UINT total = 0;
    client_ptr_t iosb_ptr = iosb_client_ptr(io);
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
    else if (type == FD_TYPE_SOCKET)
    {
        status = sock_write( handle, unix_handle, event, apc, apc_user, io, buffer, length );
        if (needs_close) close( unix_handle );
        return status;
    }

    for (;;)
    {
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
                req->async  = server_async( handle, &fileio->io, event, apc, apc_user, iosb_ptr );
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
        set_sync_iosb( io, status, total, options );
        TRACE("= SUCCESS (%u)\n", total);
        if (event) NtSetEvent( event, NULL );
        if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc, (ULONG_PTR)apc_user, iosb_ptr, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }

    ret_status = async_write && type == FD_TYPE_FILE && status == STATUS_SUCCESS ? STATUS_PENDING : status;
    if (send_completion && async_write)
        add_completion( handle, cvalue, status, total, ret_status == STATUS_PENDING );
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
    unsigned int options, status;
    UINT pos = 0, total = 0;
    enum server_fd_type type;

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

 done:
    if (needs_close) close( unix_handle );
    if (status == STATUS_SUCCESS)
    {
        file_complete_async( file, options, event, apc, apc_user, io, status, total );
        TRACE("= SUCCESS (%u)\n", total);
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }
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
           handle, event, apc, apc_context, io, code,
           in_buffer, in_size, out_buffer, out_size );

    /* some broken applications call this frequently with INVALID_HANDLE_VALUE,
     * and run slowly if we make a server call every time */
    if (HandleToLong( handle ) == ~0)
        return STATUS_INVALID_HANDLE;

    switch (device)
    {
    case FILE_DEVICE_BEEP:
    case FILE_DEVICE_NETWORK:
        status = sock_ioctl( handle, event, apc, apc_context, io, code, in_buffer, in_size, out_buffer, out_size );
        break;
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
    return status;
}


/* helper for internal ioctl calls */
NTSTATUS sync_ioctl( HANDLE file, ULONG code, void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size )
{
    IO_STATUS_BLOCK32 io32;
    IO_STATUS_BLOCK io;

    /* the 32-bit iosb is filled for overlapped file handles */
    io.Pointer = &io32;
    return NtDeviceIoControlFile( file, NULL, NULL, NULL, &io, code, in_buffer, in_size, out_buffer, out_size );
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
    unsigned int options;
    int fd, needs_close;
    ULONG_PTR size = 0;
    NTSTATUS status;

    TRACE( "(%p,%p,%p,%p,%p,0x%08x,%p,0x%08x,%p,0x%08x)\n",
           handle, event, apc, apc_context, io, code,
           in_buffer, in_size, out_buffer, out_size );

    if (!io) return STATUS_INVALID_PARAMETER;

    status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, &options );
    if (status && status != STATUS_BAD_DEVICE_TYPE)
        return status;
    if (needs_close) close( fd );

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
            size = sizeof(RETRIEVAL_POINTERS_BUFFER);
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }

    case FSCTL_GET_REPARSE_POINT:
        if (out_buffer && out_size)
        {
            FIXME("FSCTL_GET_REPARSE_POINT semi-stub\n");
            status = STATUS_NOT_A_REPARSE_POINT;
        }
        else status = STATUS_INVALID_USER_BUFFER;
        break;

    case FSCTL_GET_OBJECT_ID:
    {
        FILE_OBJECTID_BUFFER *info = out_buffer;
        int fd, needs_close;
        struct stat st;

        if (out_size >= sizeof(*info))
        {
            status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL );
            if (status) break;
            fstat( fd, &st );
            if (needs_close) close( fd );
            memset( info, 0, sizeof(*info) );
            memcpy( info->ObjectId, &st.st_dev, sizeof(st.st_dev) );
            memcpy( info->ObjectId + 8, &st.st_ino, sizeof(st.st_ino) );
            size = sizeof(*info);
        }
        else status = STATUS_BUFFER_TOO_SMALL;
        break;
    }

    case FSCTL_SET_SPARSE:
        TRACE("FSCTL_SET_SPARSE: Ignoring request\n");
        status = STATUS_SUCCESS;
        break;
    default:
        return server_ioctl_file( handle, event, apc, apc_context, io, code,
                                  in_buffer, in_size, out_buffer, out_size );
    }

    if (!NT_ERROR(status) && status != STATUS_PENDING)
        file_complete_async( handle, options, event, apc, apc_context, io, status, size );
    return status;
}


/******************************************************************************
 *              NtFlushBuffersFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushBuffersFile( HANDLE handle, IO_STATUS_BLOCK *io )
{
    return NtFlushBuffersFileEx( handle, 0, NULL, 0, io );
}


/******************************************************************************
 *              NtFlushBuffersFileEx   (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushBuffersFileEx( HANDLE handle, ULONG flags, void *params, ULONG size, IO_STATUS_BLOCK *io )
{
    NTSTATUS ret;
    HANDLE wait_handle;
    enum server_fd_type type;
    int fd, needs_close;

    TRACE( "(%p,0x%08x,%p,0x%08x,%p)\n", handle, flags, params, size, io );

    if (flags) FIXME( "flags 0x%08x ignored\n", flags );
    if (params || size) FIXME( "params %p/0x%08x ignored\n", params, size );

    if (!io || !virtual_check_buffer_for_write( io, sizeof(*io) )) return STATUS_ACCESS_VIOLATION;

    ret = server_get_unix_fd( handle, FILE_WRITE_DATA, &fd, &needs_close, &type, NULL );
    if (ret == STATUS_ACCESS_DENIED)
        ret = server_get_unix_fd( handle, FILE_APPEND_DATA, &fd, &needs_close, &type, NULL );

    if (!ret && (type == FD_TYPE_FILE || type == FD_TYPE_DIR || type == FD_TYPE_CHAR))
    {
        if (fsync(fd)) ret = errno_to_status( errno );
        io->Status      = ret;
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
            req->async = server_async( handle, &async->io, NULL, NULL, NULL, iosb_client_ptr(io) );
            ret = wine_server_call( req );
            wait_handle = wine_server_ptr_handle( reply->event );
            if (wait_handle && ret != STATUS_PENDING)
            {
                io->Status      = ret;
                io->Information = 0;
            }
        }
        SERVER_END_REQ;

        if (ret != STATUS_PENDING) free( async );

        if (wait_handle) ret = wait_async( wait_handle, FALSE );
    }

    if (needs_close) close( fd );
    return ret;
}


/**************************************************************************
 *           NtCancelIoFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtCancelIoFile( HANDLE handle, IO_STATUS_BLOCK *io_status )
{
    unsigned int status;

    TRACE( "%p %p\n", handle, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( handle );
        req->only_thread = TRUE;
        if (!(status = wine_server_call( req )))
        {
            io_status->Status = status;
            io_status->Information = 0;
        }
    }
    SERVER_END_REQ;

    return status;
}


/**************************************************************************
 *           NtCancelIoFileEx   (NTDLL.@)
 */
NTSTATUS WINAPI NtCancelIoFileEx( HANDLE handle, IO_STATUS_BLOCK *io, IO_STATUS_BLOCK *io_status )
{
    unsigned int status;

    TRACE( "%p %p %p\n", handle, io, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle = wine_server_obj_handle( handle );
        req->iosb   = wine_server_client_ptr( io );
        if (!(status = wine_server_call( req )))
        {
            io_status->Status = status;
            io_status->Information = 0;
        }
    }
    SERVER_END_REQ;

    return status;
}


/**************************************************************************
 *           NtCancelSynchronousIoFile (NTDLL.@)
 */
NTSTATUS WINAPI NtCancelSynchronousIoFile( HANDLE handle, IO_STATUS_BLOCK *io, IO_STATUS_BLOCK *io_status )
{
    unsigned int status;

    TRACE( "(%p %p %p)\n", handle, io, io_status );

    SERVER_START_REQ( cancel_sync )
    {
        req->handle = wine_server_obj_handle( handle );
        req->iosb   = wine_server_client_ptr( io );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    io_status->Status = status;
    io_status->Information = 0;
    return status;
}

/******************************************************************
 *           NtLockFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtLockFile( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void* apc_user,
                            IO_STATUS_BLOCK *io_status, LARGE_INTEGER *offset,
                            LARGE_INTEGER *count, ULONG *key, BOOLEAN dont_wait, BOOLEAN exclusive )
{
    static int warn;
    unsigned int ret;
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
    unsigned int status;

    TRACE( "%p %s %s\n",
           handle, wine_dbgstr_longlong(offset->QuadPart), wine_dbgstr_longlong(count->QuadPart) );

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


static BOOL read_changes_apc( void *user, ULONG_PTR *info, unsigned int *status )
{
    struct async_fileio_read_changes *fileio = user;
    int size = 0;

    if (*status == STATUS_ALERTED)
    {
        SERVER_START_REQ( read_change )
        {
            req->handle = wine_server_obj_handle( fileio->io.handle );
            wine_server_set_reply( req, fileio->data, fileio->data_size );
            *status = wine_server_call( req );
            size = wine_server_reply_size( reply );
        }
        SERVER_END_REQ;

        if (*status == STATUS_SUCCESS && fileio->buffer)
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
                *status = STATUS_NOTIFY_ENUM_DIR;
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
            *status = STATUS_NOTIFY_ENUM_DIR;
            size = 0;
        }
    }

    *info = size;
    release_fileio( &fileio->io );
    return TRUE;
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
    unsigned int status;
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
        req->async     = server_async( handle, &fileio->io, event, apc, apc_context, iosb_client_ptr(iosb) );
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

NTSTATUS get_device_info( int fd, FILE_FS_DEVICE_INFORMATION *info )
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
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
        {
            int d_type;
            if (ioctl(fd, FIODTYPE, &d_type) == 0)
            {
                switch(d_type)
                {
                case D_TAPE:
                    info->DeviceType = FILE_DEVICE_TAPE;
                    break;
                case D_DISK:
                    info->DeviceType = FILE_DEVICE_DISK;
#if defined(__APPLE__)
                    if (major(st.st_rdev) == 3 && minor(st.st_rdev) == 2) info->DeviceType = FILE_DEVICE_NULL;
#endif
                    break;
                case D_TTY:
                    info->DeviceType = FILE_DEVICE_SERIAL_PORT;
                    break;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
                case D_MEM:
                    info->DeviceType = FILE_DEVICE_NULL;
                    break;
#endif
                }
                /* no special d_type for parallel ports */
            }
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
    unsigned int status;

    status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL );
    if (status == STATUS_BAD_DEVICE_TYPE)
    {
        struct async_irp *async;
        HANDLE wait_handle;

        if (!(async = (struct async_irp *)alloc_fileio( sizeof(*async), irp_completion, handle )))
            return STATUS_NO_MEMORY;
        async->buffer  = buffer;
        async->size    = length;

        SERVER_START_REQ( get_volume_info )
        {
            req->async = server_async( handle, &async->io, NULL, NULL, NULL, iosb_client_ptr(io) );
            req->handle = wine_server_obj_handle( handle );
            req->info_class = info_class;
            wine_server_set_reply( req, buffer, length );
            status = wine_server_call( req );
            if (status != STATUS_PENDING)
            {
                io->Status = status;
                io->Information = wine_server_reply_size( reply );
            }
            wait_handle = wine_server_ptr_handle( reply->wait );
        }
        SERVER_END_REQ;
        if (status != STATUS_PENDING) free( async );
        if (wait_handle) status = wait_async( wait_handle, FALSE );
        return status;
    }
    else if (status) return io->Status = status;

    io->Information = 0;

    switch( info_class )
    {
    case FileFsLabelInformation:
        FIXME( "%p: label info not supported\n", handle );
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case FileFsSizeInformation:
        if (length < sizeof(FILE_FS_SIZE_INFORMATION))
            status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            FILE_FS_SIZE_INFORMATION *info = buffer;
            FILE_FS_FULL_SIZE_INFORMATION full_info;

            if ((status = get_full_size_info(fd, &full_info)) == STATUS_SUCCESS)
            {
                info->TotalAllocationUnits = full_info.TotalAllocationUnits;
                info->AvailableAllocationUnits = full_info.CallerAvailableAllocationUnits;
                info->SectorsPerAllocationUnit = full_info.SectorsPerAllocationUnit;
                info->BytesPerSector = full_info.BytesPerSector;
                io->Information = sizeof(*info);
            }
        }
        break;

    case FileFsDeviceInformation:
        if (length < sizeof(FILE_FS_DEVICE_INFORMATION))
            status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            FILE_FS_DEVICE_INFORMATION *info = buffer;

            if ((status = get_device_info( fd, info )) == STATUS_SUCCESS)
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
            status = STATUS_INFO_LENGTH_MISMATCH;
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
                else if (!strcmp( stfs.f_fstypename, "msdos" )) /* FreeBSD < 5, Apple */
                    fs_type = MOUNTMGR_FS_TYPE_FAT32;
                else if (!strcmp( stfs.f_fstypename, "msdosfs" )) /* FreeBSD >= 5 */
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
        status = STATUS_SUCCESS;
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
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (get_mountmgr_fs_info( handle, fd, drive, sizeof(data) ))
        {
            status = STATUS_NOT_IMPLEMENTED;
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
        status = STATUS_SUCCESS;
        break;
    }

    case FileFsControlInformation:
        FIXME( "%p: control info not supported\n", handle );
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case FileFsFullSizeInformation:
        if (length < sizeof(FILE_FS_FULL_SIZE_INFORMATION))
            status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            FILE_FS_FULL_SIZE_INFORMATION *info = buffer;
            if ((status = get_full_size_info(fd, info)) == STATUS_SUCCESS)
                io->Information = sizeof(*info);
        }
        break;

    case FileFsFullSizeInformationEx:
        if (length < sizeof(FILE_FS_FULL_SIZE_INFORMATION_EX))
            status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            FILE_FS_FULL_SIZE_INFORMATION_EX *info = buffer;
            if ((status = get_full_size_info_ex(fd, info)) == STATUS_SUCCESS)
                io->Information = sizeof(*info);
        }
        break;

    case FileFsObjectIdInformation:
        FIXME( "%p: object id info not supported\n", handle );
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case FileFsMaximumInformation:
        FIXME( "%p: maximum info not supported\n", handle );
        status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    if (needs_close) close( fd );
    return io->Status = status;
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
    int fd, needs_close;
    NTSTATUS status;

    FIXME( "(%p,%p,%p,%d,%d,%p,%d,%p,%d) semi-stub\n",
           handle, io, buffer, length, single_entry, list, list_len, index, restart );

    if ((status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
        return status;

    if (buffer && length)
        memset( buffer, 0, length );

    if (needs_close) close( fd );
    return STATUS_NO_EAS_ON_FILE;
}


/******************************************************************
 *           NtSetEaFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetEaFile( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length )
{
    FIXME( "(%p,%p,%p,%d) stub\n", handle, io, buffer, length );
    return STATUS_ACCESS_DENIED;
}


/* convert type information from server format; helper for NtQueryObject */
static void *put_object_type_info( OBJECT_TYPE_INFORMATION *p, struct object_type_info *info )
{
    const ULONG align = sizeof(DWORD_PTR) - 1;

    memset( p, 0, sizeof(*p) );
    p->TypeName.Buffer               = (WCHAR *)(p + 1);
    p->TypeName.Length               = info->name_len;
    p->TypeName.MaximumLength        = info->name_len + sizeof(WCHAR);
    p->TotalNumberOfObjects          = info->obj_count;
    p->TotalNumberOfHandles          = info->handle_count;
    p->HighWaterNumberOfObjects      = info->obj_max;
    p->HighWaterNumberOfHandles      = info->handle_max;
    p->TypeIndex                     = info->index + 2;
    p->GenericMapping.GenericRead    = info->mapping.read;
    p->GenericMapping.GenericWrite   = info->mapping.write;
    p->GenericMapping.GenericExecute = info->mapping.exec;
    p->GenericMapping.GenericAll     = info->mapping.all;
    p->ValidAccessMask               = info->valid_access;
    memcpy( p->TypeName.Buffer, info + 1, info->name_len );
    p->TypeName.Buffer[info->name_len / sizeof(WCHAR)] = 0;
    return (char *)(p + 1) + ((p->TypeName.MaximumLength + align) & ~align);
}

/**************************************************************************
 *           NtQueryObject   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryObject( HANDLE handle, OBJECT_INFORMATION_CLASS info_class,
                               void *ptr, ULONG len, ULONG *used_len )
{
    unsigned int status;

    TRACE("(%p,0x%08x,%p,0x%08x,%p)\n", handle, info_class, ptr, len, used_len);

    if (used_len) *used_len = 0;

    switch (info_class)
    {
    case ObjectBasicInformation:
    {
        OBJECT_BASIC_INFORMATION *p = ptr;

        if (len < sizeof(*p)) return STATUS_INFO_LENGTH_MISMATCH;

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

        SERVER_START_REQ( get_object_name )
        {
            req->handle = wine_server_obj_handle( handle );
            if (len > sizeof(*p) + sizeof(WCHAR))
                wine_server_set_reply( req, p + 1, len - sizeof(*p) - sizeof(WCHAR) );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                if (!reply->total)  /* no name */
                {
                    if (len < sizeof(*p)) status = STATUS_INFO_LENGTH_MISMATCH;
                    else memset( p, 0, sizeof(*p) );
                    if (used_len) *used_len = sizeof(*p);
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
            else if (status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_BUFFER_OVERFLOW)
            {
                if (len < sizeof(*p)) status = STATUS_INFO_LENGTH_MISMATCH;
                if (used_len) *used_len = sizeof(*p) + reply->total + sizeof(WCHAR);
            }
        }
        SERVER_END_REQ;
        break;
    }

    case ObjectTypeInformation:
    {
        OBJECT_TYPE_INFORMATION *p = ptr;
        char buffer[sizeof(struct object_type_info) + 64];
        struct object_type_info *info = (struct object_type_info *)buffer;

        SERVER_START_REQ( get_object_type )
        {
            req->handle = wine_server_obj_handle( handle );
            wine_server_set_reply( req, buffer, sizeof(buffer) );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        if (status) break;
        if (sizeof(*p) + info->name_len + sizeof(WCHAR) <= len)
        {
            put_object_type_info( p, info );
            if (used_len) *used_len = sizeof(*p) + p->TypeName.MaximumLength;
        }
        else
        {
            if (used_len) *used_len = sizeof(*p) + info->name_len + sizeof(WCHAR);
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    }

    case ObjectTypesInformation:
    {
        OBJECT_TYPES_INFORMATION *types = ptr;
        OBJECT_TYPE_INFORMATION *p;
        struct object_type_info *buffer;
        /* assume at most 32 types, with an average 16-char name */
        UINT size = 32 * (sizeof(struct object_type_info) + 16 * sizeof(WCHAR));
        UINT i, count, pos, total, align = sizeof(DWORD_PTR) - 1;

        buffer = malloc( size );
        SERVER_START_REQ( get_object_types )
        {
            wine_server_set_reply( req, buffer, size );
            status = wine_server_call( req );
            count = reply->count;
        }
        SERVER_END_REQ;
        if (!status)
        {
            if (len >= sizeof(*types)) types->NumberOfTypes = count;
            total = (sizeof(*types) + align) & ~align;
            p = (OBJECT_TYPE_INFORMATION *)((char *)ptr + total);
            for (i = pos = 0; i < count; i++)
            {
                struct object_type_info *info = (struct object_type_info *)((char *)buffer + pos);
                pos += sizeof(*info) + ((info->name_len + 3) & ~3);
                total += sizeof(*p) + ((info->name_len + sizeof(WCHAR) + align) & ~align);
                if (total <= len) p = put_object_type_info( p, info );
            }
            if (used_len) *used_len = total;
            if (total > len) status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else if (status == STATUS_BUFFER_OVERFLOW) FIXME( "size %u too small\n", size );

        free( buffer );
        break;
    }

    case ObjectHandleFlagInformation:
    {
        OBJECT_HANDLE_FLAG_INFORMATION* p = ptr;

        if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

        SERVER_START_REQ( set_handle_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->flags  = 0;
            req->mask   = 0;
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                p->Inherit = (reply->old_flags & HANDLE_FLAG_INHERIT) != 0;
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
    unsigned int status;

    TRACE("(%p,0x%08x,%p,0x%08x)\n", handle, info_class, ptr, len);

    switch (info_class)
    {
    case ObjectHandleFlagInformation:
    {
        OBJECT_HANDLE_FLAG_INFORMATION* p = ptr;

        if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

        SERVER_START_REQ( set_handle_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->mask   = HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE;
            if (p->Inherit) req->flags |= HANDLE_FLAG_INHERIT;
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
