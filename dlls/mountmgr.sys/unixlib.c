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
#include <unistd.h>

#include <pthread.h>

#include <wine/list.h>

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

static pthread_mutex_t device_op_cs = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t device_op_cv = PTHREAD_COND_INITIALIZER;
static struct list device_op_list = LIST_INIT( device_op_list );
struct device_info_list
{
    struct device_info device_info;
    struct list entry;
};

void queue_device_op( enum device_op op, const char *udi, const char *device,
                      const char *mount_point, enum device_type type, const GUID *guid,
                      const char *serial, const struct scsi_info *scsi_info )
{
    struct device_info_list *info;
    char *str, *end;

    info = calloc( 1, sizeof(*info) );
    str = info->device_info.str_buffer;
    end = info->device_info.str_buffer + sizeof(info->device_info.str_buffer);
    info->device_info.op = op;
    info->device_info.type = type;
#define ADD_STR(s) if (s && str + strlen(s) + 1 <= end) \
    { \
        info->device_info.s = strcpy( str, s ); \
        str += strlen(str) + 1; \
    }
    ADD_STR(udi);
    ADD_STR(device);
    ADD_STR(mount_point);
    ADD_STR(serial);
#undef ADD_STR
    if (guid)
    {
        info->device_info.guid_buffer = *guid;
        info->device_info.guid = &info->device_info.guid_buffer;
    }
    if (scsi_info)
    {
        info->device_info.scsi_buffer = *scsi_info;
        info->device_info.scsi_info = &info->device_info.scsi_buffer;
    }

    pthread_mutex_lock(&device_op_cs);
    list_add_tail(&device_op_list, &info->entry);
    pthread_cond_signal(&device_op_cv);
    pthread_mutex_unlock(&device_op_cs);
}

static NTSTATUS run_loop( void *args )
{
    run_diskarbitration_loop();
    run_dbus_loop();
    return STATUS_SUCCESS;
}

static NTSTATUS dequeue_device_op( void *args )
{
    const struct dequeue_device_op_params *params = args;
    struct device_info *dst = params->info;
    struct device_info_list *src;

    pthread_mutex_lock(&device_op_cs);
    while (list_empty(&device_op_list))
        pthread_cond_wait(&device_op_cv, &device_op_cs);
    src = LIST_ENTRY(list_head(&device_op_list), struct device_info_list, entry);
    list_remove(&src->entry);
    pthread_mutex_unlock(&device_op_cs);

    /* copy info to client address space and fix up pointers */
    *dst = src->device_info;
    if (dst->udi) dst->udi = (char *)dst + (src->device_info.udi - (char *)&src->device_info);
    if (dst->device) dst->device = (char *)dst + (src->device_info.device - (char *)&src->device_info);
    if (dst->mount_point) dst->mount_point = (char *)dst + (src->device_info.mount_point - (char *)&src->device_info);
    if (dst->serial) dst->serial = (char *)dst + (src->device_info.serial - (char *)&src->device_info);
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
    char *name = malloc( strlen(params->volume) + strlen(params->file) + 2 );

    sprintf( name, "%s/%s", params->volume, params->file );

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
#ifdef __ANDROID__
        home = "/sdcard/Download";
#endif
        link += 5;
        homelink = malloc( strlen(home) + strlen(link) + 1 );
        strcpy( homelink, home );
        strcat( homelink, link );
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

#ifdef _WIN64

typedef ULONG PTR32;
typedef ULONG HANDLE32;

static NTSTATUS wow64_dequeue_device_op(void *args)
{
    struct device_info32
    {
        ULONG op;
        ULONG type;
        PTR32 udi;
        PTR32 device;
        PTR32 mount_point;
        PTR32 serial;
        PTR32 guid;
        PTR32 scsi_info;

        /* buffer space for pointers */
        GUID guid_buffer;
        struct scsi_info scsi_buffer;
        char str_buffer[1024];
    };

    struct
    {
        PTR32 info;
    } *params32 = args;
    struct device_info32 *dst = UlongToPtr(params32->info);
    struct device_info src;
    struct dequeue_device_op_params params =
    {
        &src,
    };
    NTSTATUS status;

    status = dequeue_device_op(&params);
    if (status) return status;

    dst->op = src.op;
    dst->type = src.type;
    if (src.udi)
        dst->udi = PtrToUlong(dst->str_buffer + (src.udi - src.str_buffer));
    else
        dst->udi = 0;
    if (src.device)
        dst->device = PtrToUlong(dst->str_buffer + (src.device - src.str_buffer));
    else
        dst->device = 0;
    if (src.mount_point)
        dst->mount_point = PtrToUlong(dst->str_buffer + (src.mount_point - src.str_buffer));
    else
        dst->mount_point = 0;
    if (src.serial)
        dst->serial = PtrToUlong(dst->str_buffer + (src.serial - src.str_buffer));
    else
        dst->serial = 0;
    if (src.guid)
        dst->guid = PtrToUlong(&dst->guid_buffer);
    else
        dst->guid = 0;
    if (src.scsi_info)
        dst->scsi_info = PtrToUlong(&dst->scsi_info);
    else
        dst->scsi_info = 0;
    dst->guid_buffer = src.guid_buffer;
    dst->scsi_buffer = src.scsi_buffer;
    memcpy(dst->str_buffer, src.str_buffer, sizeof(dst->str_buffer));

    return STATUS_SUCCESS;
}

static NTSTATUS wow64_add_drive(void *args)
{
    struct
    {
        PTR32 device;
        ULONG type;
        PTR32 letter;
    } *params32 = args;
    struct add_drive_params params =
    {
        ULongToPtr(params32->device),
        params32->type,
        ULongToPtr(params32->letter),
    };
    return add_drive(&params);
}

static NTSTATUS wow64_get_dosdev_symlink(void *args)
{
    struct
    {
        PTR32 dev;
        PTR32 dest;
        ULONG size;
    } *params32 = args;
    struct get_dosdev_symlink_params params =
    {
        ULongToPtr(params32->dev),
        ULongToPtr(params32->dest),
        params32->size,
    };
    return get_dosdev_symlink(&params);
}

static NTSTATUS wow64_set_dosdev_symlink(void *args)
{
    struct
    {
        PTR32 dev;
        PTR32 dest;
    } *params32 = args;
    struct set_dosdev_symlink_params params =
    {
        ULongToPtr(params32->dev),
        ULongToPtr(params32->dest),
    };
    return set_dosdev_symlink(&params);
}

static NTSTATUS wow64_get_volume_dos_devices(void *args)
{
    struct
    {
        PTR32 mount_point;
        PTR32 dosdev;
    } *params32 = args;
    struct get_volume_dos_devices_params params =
    {
        ULongToPtr(params32->mount_point),
        ULongToPtr(params32->dosdev),
    };
    return get_volume_dos_devices(&params);
}

static NTSTATUS wow64_read_volume_file(void *args)
{
    struct
    {
        PTR32 volume;
        PTR32 file;
        PTR32 buffer;
        PTR32 size;
    } *params32 = args;
    struct read_volume_file_params params =
    {
        ULongToPtr(params32->volume),
        ULongToPtr(params32->file),
        ULongToPtr(params32->buffer),
        ULongToPtr(params32->size),
    };
    return read_volume_file(&params);
}

static NTSTATUS wow64_match_unixdev(void *args)
{
    struct
    {
        PTR32 device;
        ULONGLONG unix_dev;
    } *params32 = args;
    struct match_unixdev_params params =
    {
        ULongToPtr(params32->device),
        params32->unix_dev,
    };
    return match_unixdev(&params);
}

static NTSTATUS wow64_check_device_access(void *args)
{
    struct
    {
        PTR32 unix_device;
    } *params32 = args;
    return check_device_access(ULongToPtr(params32->unix_device));
}

static NTSTATUS wow64_detect_serial_ports(void *args)
{
    struct
    {
        PTR32 names;
        ULONG size;
    } *params32 = args;
    struct detect_ports_params params =
    {
        ULongToPtr(params32->names),
        params32->size,
    };
    return detect_serial_ports(&params);
}

static NTSTATUS wow64_detect_parallel_ports(void *args)
{
    struct
    {
        PTR32 names;
        ULONG size;
    } *params32 = args;
    struct detect_ports_params params =
    {
        ULongToPtr(params32->names),
        params32->size,
    };
    return detect_parallel_ports(&params);
}

static NTSTATUS wow64_set_shell_folder(void *args)
{
    struct
    {
        PTR32 folder;
        PTR32 backup;
        PTR32 link;
    } *params32 = args;
    struct set_shell_folder_params params =
    {
        ULongToPtr(params32->folder),
        ULongToPtr(params32->backup),
        ULongToPtr(params32->link),
    };
    return set_shell_folder(&params);
}

static NTSTATUS wow64_get_shell_folder(void *args)
{
    struct
    {
        PTR32 folder;
        PTR32 buffer;
        ULONG size;
    } *params32 = args;
    struct get_shell_folder_params params =
    {
        ULongToPtr(params32->folder),
        ULongToPtr(params32->buffer),
        params32->size,
    };
    return get_shell_folder(&params);
}

static NTSTATUS wow64_dhcp_request(void *args)
{
    struct
    {
        PTR32 unix_name;
        PTR32 req;
        PTR32 buffer;
        ULONG offset;
        ULONG size;
        PTR32 ret_size;
    } *params32 = args;
    struct dhcp_request_params params =
    {
        ULongToPtr(params32->unix_name),
        ULongToPtr(params32->req),
        ULongToPtr(params32->buffer),
        params32->offset,
        params32->size,
        ULongToPtr(params32->ret_size),
    };
    return dhcp_request(&params);
}

static void wow64_get_ioctl_params(void *args, struct ioctl_params *params)
{
    struct
    {
        PTR32 buff;
        ULONG insize;
        ULONG outsize;
        PTR32 info;
    } *params32 = args;

    params->buff = ULongToPtr(params32->buff);
    params->insize = params32->insize;
    params->outsize = params32->outsize;
    params->info = ULongToPtr(params32->info);
}

static NTSTATUS wow64_query_symbol_file(void *args)
{
    struct ioctl_params params;
    wow64_get_ioctl_params(args, &params);
    return query_symbol_file(&params);
}

static NTSTATUS wow64_read_credential(void *args)
{
    struct ioctl_params params;
    wow64_get_ioctl_params(args, &params);
    return read_credential(&params);
}

static NTSTATUS wow64_write_credential(void *args)
{
    struct ioctl_params params;
    wow64_get_ioctl_params(args, &params);
    return write_credential(&params);
}

static NTSTATUS wow64_delete_credential(void *args)
{
    struct ioctl_params params;
    wow64_get_ioctl_params(args, &params);
    return delete_credential(&params);
}

static NTSTATUS wow64_enumerate_credentials(void *args)
{
    struct ioctl_params params;
    wow64_get_ioctl_params(args, &params);
    return enumerate_credentials(&params);
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    run_loop,
    wow64_dequeue_device_op,
    wow64_add_drive,
    wow64_get_dosdev_symlink,
    wow64_set_dosdev_symlink,
    wow64_get_volume_dos_devices,
    wow64_read_volume_file,
    wow64_match_unixdev,
    wow64_check_device_access,
    wow64_detect_serial_ports,
    wow64_detect_parallel_ports,
    wow64_set_shell_folder,
    wow64_get_shell_folder,
    wow64_dhcp_request,
    wow64_query_symbol_file,
    wow64_read_credential,
    wow64_write_credential,
    wow64_delete_credential,
    wow64_enumerate_credentials,
};

#endif  /* _WIN64 */
