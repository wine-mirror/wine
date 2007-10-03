/*
 * Server-side file management
 *
 * Copyright (C) 1998 Alexandre Julliard
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
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "file.h"
#include "handle.h"
#include "thread.h"
#include "request.h"
#include "process.h"
#include "security.h"

struct file
{
    struct object       obj;        /* object header */
    struct fd          *fd;         /* file descriptor for this file */
    unsigned int        access;     /* file access (FILE_READ_DATA etc.) */
    mode_t              mode;       /* file stat.st_mode */
    uid_t               uid;        /* file stat.st_uid */
};

static unsigned int generic_file_map_access( unsigned int access );

static void file_dump( struct object *obj, int verbose );
static struct fd *file_get_fd( struct object *obj );
static struct security_descriptor *file_get_sd( struct object *obj );
static int file_set_sd( struct object *obj, const struct security_descriptor *sd, unsigned int set_info );
static void file_destroy( struct object *obj );

static int file_get_poll_events( struct fd *fd );
static void file_flush( struct fd *fd, struct event **event );
static enum server_fd_type file_get_fd_type( struct fd *fd );

static const struct object_ops file_ops =
{
    sizeof(struct file),          /* size */
    file_dump,                    /* dump */
    add_queue,                    /* add_queue */
    remove_queue,                 /* remove_queue */
    default_fd_signaled,          /* signaled */
    no_satisfied,                 /* satisfied */
    no_signal,                    /* signal */
    file_get_fd,                  /* get_fd */
    default_fd_map_access,        /* map_access */
    file_get_sd,                  /* get_sd */
    file_set_sd,                  /* set_sd */
    no_lookup_name,               /* lookup_name */
    no_open_file,                 /* open_file */
    fd_close_handle,              /* close_handle */
    file_destroy                  /* destroy */
};

static const struct fd_ops file_fd_ops =
{
    file_get_poll_events,         /* get_poll_events */
    default_poll_event,           /* poll_event */
    file_flush,                   /* flush */
    file_get_fd_type,             /* get_fd_type */
    default_fd_ioctl,             /* ioctl */
    default_fd_queue_async,       /* queue_async */
    default_fd_reselect_async,    /* reselect_async */
    default_fd_cancel_async       /* cancel_async */
};

static inline int is_overlapped( const struct file *file )
{
    return !(get_fd_options( file->fd ) & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));
}

/* create a file from a file descriptor */
/* if the function fails the fd is closed */
static struct file *create_file_for_fd( int fd, unsigned int access, unsigned int sharing )
{
    struct file *file;
    struct stat st;

    if (fstat( fd, &st ) == -1)
    {
        file_set_error();
        return NULL;
    }

    if ((file = alloc_object( &file_ops )))
    {
        file->mode = st.st_mode;
        file->access = default_fd_map_access( &file->obj, access );
        if (!(file->fd = create_anonymous_fd( &file_fd_ops, fd, &file->obj,
                                              FILE_SYNCHRONOUS_IO_NONALERT )))
        {
            release_object( file );
            return NULL;
        }
    }
    return file;
}

static struct object *create_file_obj( struct fd *fd, unsigned int access, mode_t mode )
{
    struct file *file = alloc_object( &file_ops );

    if (!file) return NULL;
    file->access  = access;
    file->mode    = mode;
    file->fd      = fd;
    grab_object( fd );
    set_fd_user( fd, &file_fd_ops, &file->obj );
    return &file->obj;
}

static struct object *create_file( const char *nameptr, data_size_t len, unsigned int access,
                                   unsigned int sharing, int create, unsigned int options,
                                   unsigned int attrs )
{
    struct object *obj = NULL;
    struct fd *fd;
    int flags;
    char *name;
    mode_t mode;

    if (!(name = mem_alloc( len + 1 ))) return NULL;
    memcpy( name, nameptr, len );
    name[len] = 0;

    switch(create)
    {
    case FILE_CREATE:       flags = O_CREAT | O_EXCL; break;
    case FILE_OVERWRITE_IF: /* FIXME: the difference is whether we trash existing attr or not */
    case FILE_SUPERSEDE:    flags = O_CREAT | O_TRUNC; break;
    case FILE_OPEN:         flags = 0; break;
    case FILE_OPEN_IF:      flags = O_CREAT; break;
    case FILE_OVERWRITE:    flags = O_TRUNC; break;
    default:                set_error( STATUS_INVALID_PARAMETER ); goto done;
    }

    mode = (attrs & FILE_ATTRIBUTE_READONLY) ? 0444 : 0666;

    if (len >= 4 &&
        (!strcasecmp( name + len - 4, ".exe" ) || !strcasecmp( name + len - 4, ".com" )))
        mode |= 0111;

    access = generic_file_map_access( access );

    /* FIXME: should set error to STATUS_OBJECT_NAME_COLLISION if file existed before */
    fd = open_fd( name, flags | O_NONBLOCK | O_LARGEFILE, &mode, access, sharing, options );
    if (!fd) goto done;

    if (S_ISDIR(mode))
        obj = create_dir_obj( fd );
    else if (S_ISCHR(mode) && is_serial_fd( fd ))
        obj = create_serial( fd );
    else
        obj = create_file_obj( fd, access, mode );

    release_object( fd );

done:
    free( name );
    return obj;
}

/* check if two file objects point to the same file */
int is_same_file( struct file *file1, struct file *file2 )
{
    return is_same_file_fd( file1->fd, file2->fd );
}

/* create a temp file for anonymous mappings */
struct file *create_temp_file( int access )
{
    char tmpfn[16];
    int fd;

    sprintf( tmpfn, "anonmap.XXXXXX" );  /* create it in the server directory */
    fd = mkstemps( tmpfn, 0 );
    if (fd == -1)
    {
        file_set_error();
        return NULL;
    }
    unlink( tmpfn );
    return create_file_for_fd( fd, access, 0 );
}

static void file_dump( struct object *obj, int verbose )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    fprintf( stderr, "File fd=%p\n", file->fd );
}

static int file_get_poll_events( struct fd *fd )
{
    struct file *file = get_fd_user( fd );
    int events = 0;
    assert( file->obj.ops == &file_ops );
    if (file->access & FILE_UNIX_READ_ACCESS) events |= POLLIN;
    if (file->access & FILE_UNIX_WRITE_ACCESS) events |= POLLOUT;
    return events;
}

static void file_flush( struct fd *fd, struct event **event )
{
    int unix_fd = get_unix_fd( fd );
    if (unix_fd != -1 && fsync( unix_fd ) == -1) file_set_error();
}

static enum server_fd_type file_get_fd_type( struct fd *fd )
{
    struct file *file = get_fd_user( fd );

    if (S_ISREG(file->mode) || S_ISBLK(file->mode)) return FD_TYPE_FILE;
    if (S_ISDIR(file->mode)) return FD_TYPE_DIR;
    return FD_TYPE_CHAR;
}

static struct fd *file_get_fd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );
    return (struct fd *)grab_object( file->fd );
}

static unsigned int generic_file_map_access( unsigned int access )
{
    if (access & GENERIC_READ)    access |= FILE_GENERIC_READ;
    if (access & GENERIC_WRITE)   access |= FILE_GENERIC_WRITE;
    if (access & GENERIC_EXECUTE) access |= FILE_GENERIC_EXECUTE;
    if (access & GENERIC_ALL)     access |= FILE_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static struct security_descriptor *file_get_sd( struct object *obj )
{
    struct file *file = (struct file *)obj;
    struct stat st;
    int unix_fd;
    struct security_descriptor *sd;
    const SID *user;
    const SID *group;
    size_t dacl_size;
    ACCESS_ALLOWED_ACE *aaa;
    ACL *dacl;
    SID *sid;
    char *ptr;
    const SID *world_sid = security_world_sid;
    const SID *local_system_sid = security_local_system_sid;

    assert( obj->ops == &file_ops );

    unix_fd = get_file_unix_fd( file );

    if (unix_fd == -1) return obj->sd;

    if (fstat( unix_fd, &st ) == -1)
        return obj->sd;

    /* mode and uid the same? if so, no need to re-generate security descriptor */
    if (obj->sd && (st.st_mode & (S_IRWXU|S_IRWXO)) == (file->mode & (S_IRWXU|S_IRWXO)) &&
        (st.st_uid == file->uid))
        return obj->sd;

    user = security_unix_uid_to_sid( st.st_uid );
    group = token_get_primary_group( current->process->token );

    dacl_size = sizeof(ACL) + FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
        FIELD_OFFSET(SID, SubAuthority[local_system_sid->SubAuthorityCount]);
    if (st.st_mode & S_IRWXU)
        dacl_size += FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
            FIELD_OFFSET(SID, SubAuthority[user->SubAuthorityCount]);
    if (st.st_mode & S_IRWXO)
        dacl_size += FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
            FIELD_OFFSET(SID, SubAuthority[world_sid->SubAuthorityCount]);

    sd = mem_alloc( sizeof(struct security_descriptor) +
                    FIELD_OFFSET(SID, SubAuthority[user->SubAuthorityCount]) +
                    FIELD_OFFSET(SID, SubAuthority[group->SubAuthorityCount]) +
                    dacl_size );
    if (!sd) return obj->sd;

    sd->control = SE_DACL_PRESENT;
    sd->owner_len = FIELD_OFFSET(SID, SubAuthority[user->SubAuthorityCount]);
    sd->group_len = FIELD_OFFSET(SID, SubAuthority[group->SubAuthorityCount]);
    sd->sacl_len = 0;
    sd->dacl_len = dacl_size;

    ptr = (char *)(sd + 1);
    memcpy( ptr, user, sd->owner_len );
    ptr += sd->owner_len;
    memcpy( ptr, group, sd->group_len );
    ptr += sd->group_len;

    dacl = (ACL *)ptr;
    dacl->AclRevision = ACL_REVISION;
    dacl->Sbz1 = 0;
    dacl->AclSize = dacl_size;
    dacl->AceCount = 1 + (st.st_mode & S_IRWXU ? 1 : 0) + (st.st_mode & S_IRWXO ? 1 : 0);
    dacl->Sbz2 = 0;

    /* always give FILE_ALL_ACCESS for Local System */
    aaa = (ACCESS_ALLOWED_ACE *)(dacl + 1);
    aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    aaa->Header.AceFlags = 0;
    aaa->Header.AceSize = FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
        FIELD_OFFSET(SID, SubAuthority[local_system_sid->SubAuthorityCount]);
    aaa->Mask = FILE_ALL_ACCESS;
    sid = (SID *)&aaa->SidStart;
    memcpy( sid, local_system_sid, FIELD_OFFSET(SID, SubAuthority[local_system_sid->SubAuthorityCount]) );

    if (st.st_mode & S_IRWXU)
    {
        /* appropriate access rights for the user */
        aaa = (ACCESS_ALLOWED_ACE *)ace_next( &aaa->Header );
        aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        aaa->Header.AceFlags = 0;
        aaa->Header.AceSize = FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
                              FIELD_OFFSET(SID, SubAuthority[user->SubAuthorityCount]);
        aaa->Mask = WRITE_DAC | WRITE_OWNER;
        if (st.st_mode & S_IRUSR)
            aaa->Mask |= FILE_GENERIC_READ;
        if (st.st_mode & S_IWUSR)
            aaa->Mask |= FILE_GENERIC_WRITE | DELETE;
        if (st.st_mode & S_IXUSR)
            aaa->Mask |= FILE_GENERIC_EXECUTE;
        sid = (SID *)&aaa->SidStart;
        memcpy( sid, user, FIELD_OFFSET(SID, SubAuthority[user->SubAuthorityCount]) );
    }
    if (st.st_mode & S_IRWXO)
    {
        /* appropriate access rights for Everyone */
        aaa = (ACCESS_ALLOWED_ACE *)ace_next( &aaa->Header );
        aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        aaa->Header.AceFlags = 0;
        aaa->Header.AceSize = FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
                             FIELD_OFFSET(SID, SubAuthority[world_sid->SubAuthorityCount]);
        aaa->Mask = 0;
        if (st.st_mode & S_IROTH)
            aaa->Mask |= FILE_GENERIC_READ;
        if (st.st_mode & S_IWOTH)
            aaa->Mask |= FILE_GENERIC_WRITE | DELETE;
        if (st.st_mode & S_IXOTH)
            aaa->Mask |= FILE_GENERIC_EXECUTE;
        sid = (SID *)&aaa->SidStart;
        memcpy( sid, world_sid, FIELD_OFFSET(SID, SubAuthority[world_sid->SubAuthorityCount]) );
    }

    file->mode = st.st_mode;
    file->uid = st.st_uid;
    free( obj->sd );
    obj->sd = sd;
    return sd;
}

static int file_set_sd( struct object *obj, const struct security_descriptor *sd,
                        unsigned int set_info )
{
    struct file *file = (struct file *)obj;
    mode_t new_mode;
    mode_t denied_mode = 0;
    const SID *owner;
    int unix_fd;

    assert( obj->ops == &file_ops );

    /* only DACL translation is currently supported */
    if (!(set_info & DACL_SECURITY_INFORMATION))
        return 1;

    unix_fd = get_file_unix_fd( file );

    if (unix_fd == -1) return 1;

    if (set_info & OWNER_SECURITY_INFORMATION)
    {
        owner = sd_get_owner( sd );
        if (!owner)
        {
            set_error( STATUS_INVALID_SECURITY_DESCR );
            return 0;
        }
        if (!obj->sd || !security_equal_sid( owner, sd_get_owner( obj->sd ) ))
        {
            /* FIXME: get Unix uid and call fchown */
        }
    }
    else if (obj->sd)
        owner = sd_get_owner( obj->sd );
    else
        owner = token_get_user( current->process->token );

    /* keep the bits that we don't map to access rights in the ACL */
    new_mode = file->mode & (S_ISUID|S_ISGID|S_ISVTX|S_IRWXG);

    if (set_info & DACL_SECURITY_INFORMATION)
    {
        if (sd->control & SE_DACL_PRESENT)
        {
            const ACL *dacl = (const ACL *)((char *)sd + sd->owner_len + sd->group_len + sd->sacl_len);
            const ACE_HEADER *ace = (const ACE_HEADER *)(dacl + 1);
            ULONG i;
            for (i = 0; i < dacl->AceCount; i++)
            {
                const ACCESS_ALLOWED_ACE *aa_ace;
                const ACCESS_DENIED_ACE *ad_ace;
                const SID *sid;
                switch (ace->AceType)
                {
                case ACCESS_DENIED_ACE_TYPE:
                    ad_ace = (const ACCESS_DENIED_ACE *)ace;
                    sid = (const SID *)&ad_ace->SidStart;
                    if (security_equal_sid( sid, security_world_sid ))
                    {
                        unsigned int access = generic_file_map_access( ad_ace->Mask );
                        if (access & FILE_READ_DATA)
                            denied_mode |= S_IROTH;
                        if (access & FILE_WRITE_DATA)
                            denied_mode |= S_IWOTH;
                        if (access & FILE_EXECUTE)
                            denied_mode |= S_IXOTH;
                    }
                    else if (security_equal_sid( sid, owner ))
                    {
                        unsigned int access = generic_file_map_access( ad_ace->Mask );
                        if (access & FILE_READ_DATA)
                            denied_mode |= S_IRUSR;
                        if (access & FILE_WRITE_DATA)
                            denied_mode |= S_IWUSR;
                        if (access & FILE_EXECUTE)
                            denied_mode |= S_IXUSR;
                    }
                    break;
                case ACCESS_ALLOWED_ACE_TYPE:
                    aa_ace = (const ACCESS_ALLOWED_ACE *)ace;
                    sid = (const SID *)&aa_ace->SidStart;
                    if (security_equal_sid( sid, security_world_sid ))
                    {
                        unsigned int access = generic_file_map_access( aa_ace->Mask );
                        if (access & FILE_READ_DATA)
                            new_mode |= S_IROTH;
                        if (access & FILE_WRITE_DATA)
                            new_mode |= S_IWOTH;
                        if (access & FILE_EXECUTE)
                            new_mode |= S_IXOTH;
                    }
                    else if (security_equal_sid( sid, owner ))
                    {
                        unsigned int access = generic_file_map_access( aa_ace->Mask );
                        if (access & FILE_READ_DATA)
                            new_mode |= S_IRUSR;
                        if (access & FILE_WRITE_DATA)
                            new_mode |= S_IWUSR;
                        if (access & FILE_EXECUTE)
                            new_mode |= S_IXUSR;
                    }
                    break;
                }
                ace = ace_next( ace );
            }
        }
        else
            /* no ACL means full access rights to anyone */
            new_mode |= S_IRWXU | S_IRWXO;

        if (fchmod( unix_fd, new_mode & ~denied_mode ) == -1)
        {
            file_set_error();
            return 0;
        }

        file->mode = new_mode & ~denied_mode;
    }
    return 1;
}

static void file_destroy( struct object *obj )
{
    struct file *file = (struct file *)obj;
    assert( obj->ops == &file_ops );

    if (file->fd) release_object( file->fd );
}

/* set the last error depending on errno */
void file_set_error(void)
{
    switch (errno)
    {
    case ETXTBSY:
    case EAGAIN:    set_error( STATUS_SHARING_VIOLATION ); break;
    case EBADF:     set_error( STATUS_INVALID_HANDLE ); break;
    case ENOSPC:    set_error( STATUS_DISK_FULL ); break;
    case EACCES:
    case ESRCH:
    case EPERM:     set_error( STATUS_ACCESS_DENIED ); break;
    case EROFS:     set_error( STATUS_MEDIA_WRITE_PROTECTED ); break;
    case EBUSY:     set_error( STATUS_FILE_LOCK_CONFLICT ); break;
    case ENOENT:    set_error( STATUS_NO_SUCH_FILE ); break;
    case EISDIR:    set_error( STATUS_FILE_IS_A_DIRECTORY ); break;
    case ENFILE:
    case EMFILE:    set_error( STATUS_TOO_MANY_OPENED_FILES ); break;
    case EEXIST:    set_error( STATUS_OBJECT_NAME_COLLISION ); break;
    case EINVAL:    set_error( STATUS_INVALID_PARAMETER ); break;
    case ESPIPE:    set_error( STATUS_ILLEGAL_FUNCTION ); break;
    case ENOTEMPTY: set_error( STATUS_DIRECTORY_NOT_EMPTY ); break;
    case EIO:       set_error( STATUS_ACCESS_VIOLATION ); break;
    case ENOTDIR:   set_error( STATUS_NOT_A_DIRECTORY ); break;
    case EFBIG:     set_error( STATUS_SECTION_TOO_BIG ); break;
    case ENODEV:    set_error( STATUS_NO_SUCH_DEVICE ); break;
    case ENXIO:     set_error( STATUS_NO_SUCH_DEVICE ); break;
#ifdef EOVERFLOW
    case EOVERFLOW: set_error( STATUS_INVALID_PARAMETER ); break;
#endif
    default:
        perror("wineserver: file_set_error() can't map error");
        set_error( STATUS_UNSUCCESSFUL );
        break;
    }
}

struct file *get_file_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct file *)get_handle_obj( process, handle, access, &file_ops );
}

int get_file_unix_fd( struct file *file )
{
    return get_unix_fd( file->fd );
}

struct file *grab_file_unless_removable( struct file *file )
{
    if (is_fd_removable( file->fd )) return NULL;
    return (struct file *)grab_object( file );
}

/* extend a file beyond the current end of file */
static int extend_file( struct file *file, file_pos_t new_size )
{
    static const char zero;
    int unix_fd = get_file_unix_fd( file );
    off_t size = new_size;

    if (unix_fd == -1) return 0;

    if (sizeof(new_size) > sizeof(size) && size != new_size)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return 0;
    }
    /* extend the file one byte beyond the requested size and then truncate it */
    /* this should work around ftruncate implementations that can't extend files */
    if (pwrite( unix_fd, &zero, 1, size ) != -1)
    {
        ftruncate( unix_fd, size );
        return 1;
    }
    file_set_error();
    return 0;
}

/* try to grow the file to the specified size */
int grow_file( struct file *file, file_pos_t size )
{
    struct stat st;
    int unix_fd = get_file_unix_fd( file );

    if (unix_fd == -1) return 0;

    if (fstat( unix_fd, &st ) == -1)
    {
        file_set_error();
        return 0;
    }
    if (st.st_size >= size) return 1;  /* already large enough */
    return extend_file( file, size );
}

/* create a file */
DECL_HANDLER(create_file)
{
    struct object *file;

    reply->handle = 0;
    if ((file = create_file( get_req_data(), get_req_data_size(), req->access,
                             req->sharing, req->create, req->options, req->attrs )))
    {
        reply->handle = alloc_handle( current->process, file, req->access, req->attributes );
        release_object( file );
    }
}

/* allocate a file handle for a Unix fd */
DECL_HANDLER(alloc_file_handle)
{
    struct file *file;
    int fd;

    reply->handle = 0;
    if ((fd = thread_get_inflight_fd( current, req->fd )) == -1)
    {
        set_error( STATUS_INVALID_HANDLE );
        return;
    }
    if ((file = create_file_for_fd( fd, req->access, FILE_SHARE_READ | FILE_SHARE_WRITE )))
    {
        reply->handle = alloc_handle( current->process, file, req->access, req->attributes );
        release_object( file );
    }
}

/* lock a region of a file */
DECL_HANDLER(lock_file)
{
    struct file *file;
    file_pos_t offset = ((file_pos_t)req->offset_high << 32) | req->offset_low;
    file_pos_t count = ((file_pos_t)req->count_high << 32) | req->count_low;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        reply->handle = lock_fd( file->fd, offset, count, req->shared, req->wait );
        reply->overlapped = is_overlapped( file );
        release_object( file );
    }
}

/* unlock a region of a file */
DECL_HANDLER(unlock_file)
{
    struct file *file;
    file_pos_t offset = ((file_pos_t)req->offset_high << 32) | req->offset_low;
    file_pos_t count = ((file_pos_t)req->count_high << 32) | req->count_low;

    if ((file = get_file_obj( current->process, req->handle, 0 )))
    {
        unlock_fd( file->fd, offset, count );
        release_object( file );
    }
}
