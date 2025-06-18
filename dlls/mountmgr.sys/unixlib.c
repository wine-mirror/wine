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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#include <unistd.h>

#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

static struct run_loop_params run_loop_params;

static NTSTATUS errno_to_status( int err )
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

static char *get_dosdevices_path( const char *dev )
{
    const char *prefix = getenv( "WINEPREFIX" );
    char *path = NULL;

    if (prefix)
        asprintf( &path, "%s/dosdevices/%s", prefix, dev );
    else
        asprintf( &path, "%s/.wine/dosdevices/%s", getenv( "HOME" ), dev );

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
            int len = snprintf( unix_path, sizeof(unix_path), *paths, i++ );
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

void queue_device_op( enum device_op op, const char *udi, const char *device,
                      const char *mount_point, enum device_type type, const GUID *guid,
                      const char *serial, const char *label, const struct scsi_info *scsi_info )
{
    struct device_info *info;
    char *str, *end;

    info = calloc( 1, sizeof(*info) );
    str = info->str_buffer;
    end = info->str_buffer + sizeof(info->str_buffer);
    info->op = op;
    info->type = type;
#define ADD_STR(s) if (s && str + strlen(s) + 1 <= end) \
    { \
        info->s = strcpy( str, s ); \
        str += strlen(str) + 1; \
    }
    ADD_STR(udi);
    ADD_STR(device);
    ADD_STR(mount_point);
    ADD_STR(serial);
    ADD_STR(label);
#undef ADD_STR
    if (guid)
    {
        info->guid_buffer = *guid;
        info->guid = &info->guid_buffer;
    }
    if (scsi_info)
    {
        info->scsi_buffer = *scsi_info;
        info->scsi_info = &info->scsi_buffer;
    }
    NtQueueApcThread( run_loop_params.op_thread, run_loop_params.op_apc, (ULONG_PTR)info, 0, 0 );
}

static NTSTATUS run_loop( void *args )
{
    const struct run_loop_params *params = args;

    run_loop_params = *params;
    run_diskarbitration_loop();
    run_dbus_loop();
    return STATUS_SUCCESS;
}

static NTSTATUS dequeue_device_op( void *args )
{
    const struct dequeue_device_op_params *params = args;
    struct device_info *src = (struct device_info *)params->arg;
    struct device_info *dst = params->info;

    /* copy info to client address space and fix up pointers */
    *dst = *src;
    if (dst->udi) dst->udi = (char *)dst + (src->udi - (char *)src);
    if (dst->device) dst->device = (char *)dst + (src->device - (char *)src);
    if (dst->mount_point) dst->mount_point = (char *)dst + (src->mount_point - (char *)src);
    if (dst->serial) dst->serial = (char *)dst + (src->serial - (char *)src);
    if (dst->label) dst->label = (char *)dst + (src->label - (char *)src);
    if (dst->guid) dst->guid = &dst->guid_buffer;
    if (dst->scsi_info) dst->scsi_info = &dst->scsi_buffer;

    free( src );
    return STATUS_SUCCESS;
}

/* find or create a DOS drive for the corresponding Unix device */
static NTSTATUS add_drive( void *args )
{
    const struct add_drive_params *params = args;
    char *path, *p;
    char in_use[26];
    struct stat dev_st, drive_st;
    int drive, first, last, avail = 0;

    if (stat( params->device, &dev_st ) == -1 || !is_valid_device( &dev_st )) return STATUS_NO_SUCH_DEVICE;

    if (!(path = get_dosdevices_path( "a::" ))) return STATUS_NO_MEMORY;
    p = path + strlen(path) - 3;

    memset( in_use, 0, sizeof(in_use) );

    switch (params->type)
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
            if (symlink( params->device, path ) != -1) goto done;
            /* failed, retry the search */
        }
    }
    free( path );
    return STATUS_OBJECT_NAME_COLLISION;

done:
    free( path );
    *params->letter = drive;
    return STATUS_SUCCESS;
}

static NTSTATUS get_dosdev_symlink( void *args )
{
    const struct get_dosdev_symlink_params *params = args;
    char *path;
    int ret;

    if (!(path = get_dosdevices_path( params->dev ))) return STATUS_NO_MEMORY;

    ret = readlink( path, params->dest, params->size );
    free( path );
    if (ret == -1) return STATUS_NO_SUCH_DEVICE;
    if (ret == params->size) return STATUS_BUFFER_TOO_SMALL;
    params->dest[ret] = 0;
    return STATUS_SUCCESS;
}

static NTSTATUS set_dosdev_symlink( void *args )
{
    const struct set_dosdev_symlink_params *params = args;
    char *path;
    NTSTATUS status = STATUS_SUCCESS;

    if (!(path = get_dosdevices_path( params->dev ))) return STATUS_NO_MEMORY;

    if (params->dest && params->dest[0])
    {
        unlink( path );
        if (symlink( params->dest, path ) == -1) status = STATUS_ACCESS_DENIED;
    }
    else unlink( path );

    free( path );
    return status;
}

static NTSTATUS get_volume_size_info( void *args )
{
    const struct get_volume_size_info_params *params = args;
    const char *unix_mount = params->unix_mount;
    struct size_info *info = params->info;

    struct stat st;
    ULONGLONG bsize;
    NTSTATUS status;
    int fd = -1;

#if !defined(linux) || !defined(HAVE_FSTATFS)
    struct statvfs stfs;
#else
    struct statfs stfs;
#endif

    if (!unix_mount) return STATUS_NO_SUCH_DEVICE;

    if (unix_mount[0] != '/')
    {
        char *path = get_dosdevices_path( unix_mount );
        if (path) fd = open( path, O_RDONLY );
        free( path );
    }
    else fd = open( unix_mount, O_RDONLY );

    if (fstat( fd, &st ) < 0)
    {
        status = errno_to_status( errno );
        goto done;
    }
    if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto done;
    }

    /* Linux's fstatvfs is buggy */
#if !defined(linux) || !defined(HAVE_FSTATFS)
    if (fstatvfs( fd, &stfs ) < 0)
    {
        status = errno_to_status( errno );
        goto done;
    }
    bsize = stfs.f_frsize;
#else
    if (fstatfs( fd, &stfs ) < 0)
    {
        status = errno_to_status( errno );
        goto done;
    }
    bsize = stfs.f_bsize;
#endif
    if (bsize == 2048)  /* assume CD-ROM */
    {
        info->bytes_per_sector = 2048;
        info->sectors_per_allocation_unit = 1;
    }
    else
    {
        info->bytes_per_sector = 512;
        info->sectors_per_allocation_unit = 8;
    }

    info->total_allocation_units =
        bsize * stfs.f_blocks / (info->bytes_per_sector * info->sectors_per_allocation_unit);
    info->caller_available_allocation_units =
        bsize * stfs.f_bavail / (info->bytes_per_sector * info->sectors_per_allocation_unit);
    info->actual_available_allocation_units =
        bsize * stfs.f_bfree / (info->bytes_per_sector * info->sectors_per_allocation_unit);

    status = STATUS_SUCCESS;

done:
    close( fd );
    return status;
}

static NTSTATUS get_volume_dos_devices( void *args )
{
    const struct get_volume_dos_devices_params *params = args;
    struct stat dev_st, drive_st;
    char *path;
    int i;

    if (stat( params->mount_point, &dev_st ) == -1) return STATUS_NO_SUCH_DEVICE;
    if (!(path = get_dosdevices_path( "a:" ))) return STATUS_NO_MEMORY;

    *params->dosdev = 0;
    for (i = 0; i < 26; i++)
    {
        path[strlen(path) - 2] = 'a' + i;
        if (stat( path, &drive_st ) != -1 && drive_st.st_rdev == dev_st.st_rdev) *params->dosdev |= 1 << i;
    }
    free( path );
    return STATUS_SUCCESS;
}

static NTSTATUS read_volume_file( void *args )
{
    const struct read_volume_file_params *params = args;
    int ret, fd = -1;
    char *name = NULL;

    asprintf( &name, "%s/%s", params->volume, params->file );

    if (name[0] != '/')
    {
        char *path = get_dosdevices_path( name );
        if (path) fd = open( path, O_RDONLY );
        free( path );
    }
    else fd = open( name, O_RDONLY );

    free( name );
    if (fd == -1) return STATUS_NO_SUCH_FILE;
    ret = read( fd, params->buffer, *params->size );
    close( fd );
    if (ret == -1) return STATUS_NO_SUCH_FILE;
    *params->size = ret;
    return STATUS_SUCCESS;
}

static NTSTATUS match_unixdev( void *args )
{
    const struct match_unixdev_params *params = args;
    struct stat st;

    return !stat( params->device, &st ) && st.st_rdev == params->unix_dev;
}

static NTSTATUS check_device_access( void *args )
{
#ifdef __APPLE__
    const char *unix_device = args;
    if (access( unix_device, R_OK )) return STATUS_ACCESS_DENIED;
#endif
    return STATUS_SUCCESS;
}

static NTSTATUS detect_serial_ports( void *args )
{
    const struct detect_ports_params *params = args;
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

    detect_devices( paths, params->names, params->size );
    return STATUS_SUCCESS;
}

static NTSTATUS detect_parallel_ports( void *args )
{
    const struct detect_ports_params *params = args;
    static const char *paths[] =
    {
#ifdef linux
        "/dev/lp%u",
#endif
        NULL
    };

    detect_devices( paths, params->names, params->size );
    return STATUS_SUCCESS;
}

static NTSTATUS set_shell_folder( void *args )
{
    const struct set_shell_folder_params *params = args;
    const char *folder = params->folder;
    const char *backup = params->backup;
    const char *link = params->link;
    struct stat st;
    const char *home;
    char *homelink = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    if (link && (!strcmp( link, "$HOME" ) || !strncmp( link, "$HOME/", 6 )) && (home = getenv( "HOME" )))
    {
        link += 5;
        asprintf( &homelink, "%s%s", home, link );
        link = homelink;
    }

    /* ignore nonexistent link targets */
    if (link && (stat( link, &st ) || !S_ISDIR( st.st_mode )))
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto done;
    }

    if (!lstat( folder, &st )) /* move old folder/link out of the way */
    {
        if (S_ISLNK( st.st_mode ))
        {
            unlink( folder );
        }
        else if (link && S_ISDIR( st.st_mode ))
        {
            if (rmdir( folder ))  /* non-empty dir, try to make a backup */
            {
                if (!backup || rename( folder, backup ))
                {
                    status = STATUS_OBJECT_NAME_COLLISION;
                    goto done;
                }
            }
        }
        else goto done; /* nothing to do, folder already exists */
    }

    if (link) symlink( link, folder );
    else
    {
        if (backup && !lstat( backup, &st ) && S_ISDIR( st.st_mode )) rename( backup, folder );
        else mkdir( folder, 0777 );
    }

done:
    free( homelink );
    return status;
}

static NTSTATUS get_shell_folder( void *args )
{
    const struct get_shell_folder_params *params = args;
    int ret = readlink( params->folder, params->buffer, params->size - 1 );

    if (ret < 0) return STATUS_OBJECT_NAME_NOT_FOUND;
    params->buffer[ret] = 0;
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    run_loop,
    dequeue_device_op,
    add_drive,
    get_dosdev_symlink,
    set_dosdev_symlink,
    get_volume_size_info,
    get_volume_dos_devices,
    read_volume_file,
    match_unixdev,
    check_device_access,
    detect_serial_ports,
    detect_parallel_ports,
    set_shell_folder,
    get_shell_folder,
    dhcp_request,
    query_symbol_file,
    read_credential,
    write_credential,
    delete_credential,
    enumerate_credentials,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );
