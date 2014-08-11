/*
 * Copyright 1999, 2000 Juergen Schmied
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_LINUX_MAJOR_H
# include <linux/major.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_VALGRIND_MEMCHECK_H
# include <valgrind/memcheck.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"

#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddk.h"
#include "ddk/ntddser.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

mode_t FILE_umask = 0;

#define SECSPERDAY         86400
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)

#define FILE_WRITE_TO_END_OF_FILE      ((LONGLONG)-1)
#define FILE_USE_FILE_POINTER_POSITION ((LONGLONG)-2)

static const WCHAR ntfsW[] = {'N','T','F','S'};

/**************************************************************************
 *                 FILE_CreateFile                    (internal)
 * Open a file.
 *
 * Parameter set fully identical with NtCreateFile
 */
static NTSTATUS FILE_CreateFile( PHANDLE handle, ACCESS_MASK access, POBJECT_ATTRIBUTES attr,
                                 PIO_STATUS_BLOCK io, PLARGE_INTEGER alloc_size,
                                 ULONG attributes, ULONG sharing, ULONG disposition,
                                 ULONG options, PVOID ea_buffer, ULONG ea_length )
{
    ANSI_STRING unix_name;
    BOOL created = FALSE;

    TRACE("handle=%p access=%08x name=%s objattr=%08x root=%p sec=%p io=%p alloc_size=%p "
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

    if (io->u.Status == STATUS_NO_SUCH_FILE &&
        disposition != FILE_OPEN && disposition != FILE_OVERWRITE)
    {
        created = TRUE;
        io->u.Status = STATUS_SUCCESS;
    }

    if (io->u.Status == STATUS_SUCCESS)
    {
        struct security_descriptor *sd;
        struct object_attributes objattr;

        objattr.rootdir = wine_server_obj_handle( attr->RootDirectory );
        objattr.name_len = 0;
        io->u.Status = NTDLL_create_struct_sd( attr->SecurityDescriptor, &sd, &objattr.sd_len );
        if (io->u.Status != STATUS_SUCCESS)
        {
            RtlFreeAnsiString( &unix_name );
            return io->u.Status;
        }

        SERVER_START_REQ( create_file )
        {
            req->access     = access;
            req->attributes = attr->Attributes;
            req->sharing    = sharing;
            req->create     = disposition;
            req->options    = options;
            req->attrs      = attributes;
            wine_server_add_data( req, &objattr, sizeof(objattr) );
            if (objattr.sd_len) wine_server_add_data( req, sd, objattr.sd_len );
            wine_server_add_data( req, unix_name.Buffer, unix_name.Length );
            io->u.Status = wine_server_call( req );
            *handle = wine_server_ptr_handle( reply->handle );
        }
        SERVER_END_REQ;
        NTDLL_free_struct_sd( sd );
        RtlFreeAnsiString( &unix_name );
    }
    else WARN("%s not found (%x)\n", debugstr_us(attr->ObjectName), io->u.Status );

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

/**************************************************************************
 *                 NtOpenFile				[NTDLL.@]
 *                 ZwOpenFile				[NTDLL.@]
 *
 * Open a file.
 *
 * PARAMS
 *  handle    [O] Variable that receives the file handle on return
 *  access    [I] Access desired by the caller to the file
 *  attr      [I] Structure describing the file to be opened
 *  io        [O] Receives details about the result of the operation
 *  sharing   [I] Type of shared access the caller requires
 *  options   [I] Options for the file open
 *
 * RETURNS
 *  Success: 0. FileHandle and IoStatusBlock are updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtOpenFile( PHANDLE handle, ACCESS_MASK access,
                            POBJECT_ATTRIBUTES attr, PIO_STATUS_BLOCK io,
                            ULONG sharing, ULONG options )
{
    return FILE_CreateFile( handle, access, attr, io, NULL, 0,
                            sharing, FILE_OPEN, options, NULL, 0 );
}

/**************************************************************************
 *		NtCreateFile				[NTDLL.@]
 *		ZwCreateFile				[NTDLL.@]
 *
 * Either create a new file or directory, or open an existing file, device,
 * directory or volume.
 *
 * PARAMS
 *	handle       [O] Points to a variable which receives the file handle on return
 *	access       [I] Desired access to the file
 *	attr         [I] Structure describing the file
 *	io           [O] Receives information about the operation on return
 *	alloc_size   [I] Initial size of the file in bytes
 *	attributes   [I] Attributes to create the file with
 *	sharing      [I] Type of shared access the caller would like to the file
 *	disposition  [I] Specifies what to do, depending on whether the file already exists
 *	options      [I] Options for creating a new file
 *	ea_buffer    [I] Pointer to an extended attributes buffer
 *	ea_length    [I] Length of ea_buffer
 *
 * RETURNS
 *  Success: 0. handle and io are updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtCreateFile( PHANDLE handle, ACCESS_MASK access, POBJECT_ATTRIBUTES attr,
                              PIO_STATUS_BLOCK io, PLARGE_INTEGER alloc_size,
                              ULONG attributes, ULONG sharing, ULONG disposition,
                              ULONG options, PVOID ea_buffer, ULONG ea_length )
{
    return FILE_CreateFile( handle, access, attr, io, alloc_size, attributes,
                            sharing, disposition, options, ea_buffer, ea_length );
}

/***********************************************************************
 *                  Asynchronous file I/O                              *
 */

struct async_fileio
{
    HANDLE              handle;
    PIO_APC_ROUTINE     apc;
    void               *apc_arg;
};

typedef struct
{
    struct async_fileio io;
    char*               buffer;
    unsigned int        already;
    unsigned int        count;
    BOOL                avail_mode;
} async_fileio_read;

typedef struct
{
    struct async_fileio io;
    const char         *buffer;
    unsigned int        already;
    unsigned int        count;
} async_fileio_write;


/* callback for file I/O user APC */
static void WINAPI fileio_apc( void *arg, IO_STATUS_BLOCK *io, ULONG reserved )
{
    struct async_fileio *async = arg;
    if (async->apc) async->apc( async->apc_arg, io, reserved );
    RtlFreeHeap( GetProcessHeap(), 0, async );
}

/***********************************************************************
 *           FILE_GetNtStatus(void)
 *
 * Retrieve the Nt Status code from errno.
 * Try to be consistent with FILE_SetDosError().
 */
NTSTATUS FILE_GetNtStatus(void)
{
    int err = errno;

    TRACE( "errno = %d\n", errno );
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

/***********************************************************************
 *             FILE_AsyncReadService      (INTERNAL)
 */
static NTSTATUS FILE_AsyncReadService(void *user, PIO_STATUS_BLOCK iosb, NTSTATUS status, void **apc)
{
    async_fileio_read *fileio = user;
    int fd, needs_close, result;

    switch (status)
    {
    case STATUS_ALERTED: /* got some new data */
        /* check to see if the data is ready (non-blocking) */
        if ((status = server_get_unix_fd( fileio->io.handle, FILE_READ_DATA, &fd,
                                          &needs_close, NULL, NULL )))
            break;

        result = read(fd, &fileio->buffer[fileio->already], fileio->count - fileio->already);
        if (needs_close) close( fd );

        if (result < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                status = STATUS_PENDING;
            else /* check to see if the transfer is complete */
                status = FILE_GetNtStatus();
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
            {
                /* if we only have to read the available data, and none is available,
                 * simply cancel the request. If data was available, it has been read
                 * while in by previous call (NtDelayExecution)
                 */
                status = (fileio->avail_mode) ? STATUS_SUCCESS : STATUS_PENDING;
            }
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
        *apc = fileio_apc;
    }
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
        }
        break;
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
    case FD_TYPE_PIPE:
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
        }
        break;
    case FD_TYPE_MAILSLOT:
    case FD_TYPE_SOCKET:
    case FD_TYPE_PIPE:
    case FD_TYPE_CHAR:
        *avail_mode = TRUE;
        break;
    default:
        *avail_mode = FALSE;
        break;
    }
    return status;
}


/******************************************************************************
 *  NtReadFile					[NTDLL.@]
 *  ZwReadFile					[NTDLL.@]
 *
 * Read from an open file handle.
 *
 * PARAMS
 *  FileHandle    [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  Event         [I] Event to signal upon completion (or NULL)
 *  ApcRoutine    [I] Callback to call upon completion (or NULL)
 *  ApcContext    [I] Context for ApcRoutine (or NULL)
 *  IoStatusBlock [O] Receives information about the operation on return
 *  Buffer        [O] Destination for the data read
 *  Length        [I] Size of Buffer
 *  ByteOffset    [O] Destination for the new file pointer position (or NULL)
 *  Key           [O] Function unknown (may be NULL)
 *
 * RETURNS
 *  Success: 0. IoStatusBlock is updated, and the Information member contains
 *           The number of bytes read.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtReadFile(HANDLE hFile, HANDLE hEvent,
                           PIO_APC_ROUTINE apc, void* apc_user,
                           PIO_STATUS_BLOCK io_status, void* buffer, ULONG length,
                           PLARGE_INTEGER offset, PULONG key)
{
    int result, unix_handle, needs_close;
    unsigned int options;
    struct io_timeouts timeouts;
    NTSTATUS status;
    ULONG total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE, async_read, timeout_init_done = FALSE;

    TRACE("(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p),partial stub!\n",
          hFile,hEvent,apc,apc_user,io_status,buffer,length,offset,key);

    if (!io_status) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( hFile, FILE_READ_DATA, &unix_handle,
                                 &needs_close, &type, &options );
    if (status) return status;

    async_read = !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));

    if (!virtual_check_buffer_for_write( buffer, length ))
    {
        status = STATUS_ACCESS_VIOLATION;
        goto done;
    }

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
            while ((result = pread( unix_handle, buffer, length, offset->QuadPart )) == -1)
            {
                if (errno != EINTR)
                {
                    status = FILE_GetNtStatus();
                    goto done;
                }
            }
            if (!async_read)
                /* update file pointer position */
                lseek( unix_handle, offset->QuadPart + result, SEEK_SET );

            total = result;
            status = (total || !length) ? STATUS_SUCCESS : STATUS_END_OF_FILE;
            goto done;
        }
    }
    else if (type == FD_TYPE_SERIAL)
    {
        if (async_read && (!offset || offset->QuadPart < 0))
        {
            status = STATUS_INVALID_PARAMETER;
            goto done;
        }
    }

    for (;;)
    {
        if ((result = read( unix_handle, (char *)buffer + total, length - total )) >= 0)
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
                    status = length ? STATUS_END_OF_FILE : STATUS_SUCCESS;
                    goto done;
                case FD_TYPE_SERIAL:
                    break;
                default:
                    status = STATUS_PIPE_BROKEN;
                    goto done;
                }
            }
            else if (type == FD_TYPE_FILE) continue;  /* no async I/O on regular files */
        }
        else if (errno != EAGAIN)
        {
            if (errno == EINTR) continue;
            if (!total) status = FILE_GetNtStatus();
            goto done;
        }

        if (async_read)
        {
            async_fileio_read *fileio;
            BOOL avail_mode;

            if ((status = get_io_avail_mode( hFile, type, &avail_mode )))
                goto err;
            if (total && avail_mode)
            {
                status = STATUS_SUCCESS;
                goto done;
            }

            if (!(fileio = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(*fileio))))
            {
                status = STATUS_NO_MEMORY;
                goto err;
            }
            fileio->io.handle  = hFile;
            fileio->io.apc     = apc;
            fileio->io.apc_arg = apc_user;
            fileio->already = total;
            fileio->count = length;
            fileio->buffer = buffer;
            fileio->avail_mode = avail_mode;

            SERVER_START_REQ( register_async )
            {
                req->type   = ASYNC_TYPE_READ;
                req->count  = length;
                req->async.handle   = wine_server_obj_handle( hFile );
                req->async.event    = wine_server_obj_handle( hEvent );
                req->async.callback = wine_server_client_ptr( FILE_AsyncReadService );
                req->async.iosb     = wine_server_client_ptr( io_status );
                req->async.arg      = wine_server_client_ptr( fileio );
                req->async.cvalue   = cvalue;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;

            if (status != STATUS_PENDING) RtlFreeHeap( GetProcessHeap(), 0, fileio );
            goto err;
        }
        else  /* synchronous read, wait for the fd to become ready */
        {
            struct pollfd pfd;
            int ret, timeout;

            if (!timeout_init_done)
            {
                timeout_init_done = TRUE;
                if ((status = get_io_timeouts( hFile, type, length, TRUE, &timeouts )))
                    goto err;
                if (hEvent) NtResetEvent( hEvent, NULL );
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
                status = FILE_GetNtStatus();
                goto done;
            }
            /* will now restart the read */
        }
    }

done:
    send_completion = cvalue != 0;

err:
    if (needs_close) close( unix_handle );
    if (status == STATUS_SUCCESS || (status == STATUS_END_OF_FILE && !async_read))
    {
        io_status->u.Status = status;
        io_status->Information = total;
        TRACE("= SUCCESS (%u)\n", total);
        if (hEvent) NtSetEvent( hEvent, NULL );
        if (apc && !status) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                              (ULONG_PTR)apc_user, (ULONG_PTR)io_status, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && hEvent) NtResetEvent( hEvent, NULL );
    }

    if (send_completion) NTDLL_AddCompletion( hFile, cvalue, status, total );

    return status;
}


/******************************************************************************
 *  NtReadFileScatter   [NTDLL.@]
 *  ZwReadFileScatter   [NTDLL.@]
 */
NTSTATUS WINAPI NtReadFileScatter( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                   PIO_STATUS_BLOCK io_status, FILE_SEGMENT_ELEMENT *segments,
                                   ULONG length, PLARGE_INTEGER offset, PULONG key )
{
    int result, unix_handle, needs_close;
    unsigned int options;
    NTSTATUS status;
    ULONG pos = 0, total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE;

    TRACE( "(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p),partial stub!\n",
           file, event, apc, apc_user, io_status, segments, length, offset, key);

    if (length % page_size) return STATUS_INVALID_PARAMETER;
    if (!io_status) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( file, FILE_READ_DATA, &unix_handle,
                                 &needs_close, &type, &options );
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
                            page_size - pos, offset->QuadPart + total );
        else
            result = read( unix_handle, (char *)segments->Buffer + pos, page_size - pos );

        if (result == -1)
        {
            if (errno == EINTR) continue;
            status = FILE_GetNtStatus();
            break;
        }
        if (!result)
        {
            status = STATUS_END_OF_FILE;
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

 error:
    if (needs_close) close( unix_handle );
    if (status == STATUS_SUCCESS)
    {
        io_status->u.Status = status;
        io_status->Information = total;
        TRACE("= SUCCESS (%u)\n", total);
        if (event) NtSetEvent( event, NULL );
        if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                   (ULONG_PTR)apc_user, (ULONG_PTR)io_status, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }

    if (send_completion) NTDLL_AddCompletion( file, cvalue, status, total );

    return status;
}


/***********************************************************************
 *             FILE_AsyncWriteService      (INTERNAL)
 */
static NTSTATUS FILE_AsyncWriteService(void *user, IO_STATUS_BLOCK *iosb, NTSTATUS status, void **apc)
{
    async_fileio_write *fileio = user;
    int result, fd, needs_close;
    enum server_fd_type type;

    switch (status)
    {
    case STATUS_ALERTED:
        /* write some data (non-blocking) */
        if ((status = server_get_unix_fd( fileio->io.handle, FILE_WRITE_DATA, &fd,
                                          &needs_close, &type, NULL )))
            break;

        if (!fileio->count && (type == FD_TYPE_MAILSLOT || type == FD_TYPE_PIPE || type == FD_TYPE_SOCKET))
            result = send( fd, fileio->buffer, 0, 0 );
        else
            result = write( fd, &fileio->buffer[fileio->already], fileio->count - fileio->already );

        if (needs_close) close( fd );

        if (result < 0)
        {
            if (errno == EAGAIN || errno == EINTR) status = STATUS_PENDING;
            else status = FILE_GetNtStatus();
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
        *apc = fileio_apc;
    }
    return status;
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
 *  NtWriteFile					[NTDLL.@]
 *  ZwWriteFile					[NTDLL.@]
 *
 * Write to an open file handle.
 *
 * PARAMS
 *  FileHandle    [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  Event         [I] Event to signal upon completion (or NULL)
 *  ApcRoutine    [I] Callback to call upon completion (or NULL)
 *  ApcContext    [I] Context for ApcRoutine (or NULL)
 *  IoStatusBlock [O] Receives information about the operation on return
 *  Buffer        [I] Source for the data to write
 *  Length        [I] Size of Buffer
 *  ByteOffset    [O] Destination for the new file pointer position (or NULL)
 *  Key           [O] Function unknown (may be NULL)
 *
 * RETURNS
 *  Success: 0. IoStatusBlock is updated, and the Information member contains
 *           The number of bytes written.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtWriteFile(HANDLE hFile, HANDLE hEvent,
                            PIO_APC_ROUTINE apc, void* apc_user,
                            PIO_STATUS_BLOCK io_status, 
                            const void* buffer, ULONG length,
                            PLARGE_INTEGER offset, PULONG key)
{
    int result, unix_handle, needs_close;
    unsigned int options;
    struct io_timeouts timeouts;
    NTSTATUS status;
    ULONG total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE, async_write, append_write = FALSE, timeout_init_done = FALSE;
    LARGE_INTEGER offset_eof;

    TRACE("(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p)!\n",
          hFile,hEvent,apc,apc_user,io_status,buffer,length,offset,key);

    if (!io_status) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( hFile, FILE_WRITE_DATA, &unix_handle,
                                 &needs_close, &type, &options );
    if (status == STATUS_ACCESS_DENIED)
    {
        status = server_get_unix_fd( hFile, FILE_APPEND_DATA, &unix_handle,
                                     &needs_close, &type, &options );
        append_write = TRUE;
    }
    if (status) return status;

    if (!virtual_check_buffer_for_read( buffer, length ))
    {
        status = STATUS_INVALID_USER_BUFFER;
        goto done;
    }

    async_write = !(options & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT));

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
                    status = FILE_GetNtStatus();
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
                    else status = FILE_GetNtStatus();
                    goto done;
                }
            }

            if (!async_write)
                /* update file pointer position */
                lseek( unix_handle, off + result, SEEK_SET );

            total = result;
            status = STATUS_SUCCESS;
            goto done;
        }
    }
    else if (type == FD_TYPE_SERIAL)
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
        if (!length && (type == FD_TYPE_MAILSLOT || type == FD_TYPE_PIPE || type == FD_TYPE_SOCKET))
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
                else status = FILE_GetNtStatus();
            }
            goto done;
        }

        if (async_write)
        {
            async_fileio_write *fileio;

            if (!(fileio = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(*fileio))))
            {
                status = STATUS_NO_MEMORY;
                goto err;
            }
            fileio->io.handle  = hFile;
            fileio->io.apc     = apc;
            fileio->io.apc_arg = apc_user;
            fileio->already = total;
            fileio->count = length;
            fileio->buffer = buffer;

            SERVER_START_REQ( register_async )
            {
                req->type   = ASYNC_TYPE_WRITE;
                req->count  = length;
                req->async.handle   = wine_server_obj_handle( hFile );
                req->async.event    = wine_server_obj_handle( hEvent );
                req->async.callback = wine_server_client_ptr( FILE_AsyncWriteService );
                req->async.iosb     = wine_server_client_ptr( io_status );
                req->async.arg      = wine_server_client_ptr( fileio );
                req->async.cvalue   = cvalue;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;

            if (status != STATUS_PENDING) RtlFreeHeap( GetProcessHeap(), 0, fileio );
            goto err;
        }
        else  /* synchronous write, wait for the fd to become ready */
        {
            struct pollfd pfd;
            int ret, timeout;

            if (!timeout_init_done)
            {
                timeout_init_done = TRUE;
                if ((status = get_io_timeouts( hFile, type, length, FALSE, &timeouts )))
                    goto err;
                if (hEvent) NtResetEvent( hEvent, NULL );
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
                status = FILE_GetNtStatus();
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
        set_pending_write( hFile );

    if (status == STATUS_SUCCESS)
    {
        io_status->u.Status = status;
        io_status->Information = total;
        TRACE("= SUCCESS (%u)\n", total);
        if (hEvent) NtSetEvent( hEvent, NULL );
        if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                   (ULONG_PTR)apc_user, (ULONG_PTR)io_status, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && hEvent) NtResetEvent( hEvent, NULL );
    }

    if (send_completion) NTDLL_AddCompletion( hFile, cvalue, status, total );

    return status;
}


/******************************************************************************
 *  NtWriteFileGather   [NTDLL.@]
 *  ZwWriteFileGather   [NTDLL.@]
 */
NTSTATUS WINAPI NtWriteFileGather( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                   PIO_STATUS_BLOCK io_status, FILE_SEGMENT_ELEMENT *segments,
                                   ULONG length, PLARGE_INTEGER offset, PULONG key )
{
    int result, unix_handle, needs_close;
    unsigned int options;
    NTSTATUS status;
    ULONG pos = 0, total = 0;
    enum server_fd_type type;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_user;
    BOOL send_completion = FALSE;

    TRACE( "(%p,%p,%p,%p,%p,%p,0x%08x,%p,%p),partial stub!\n",
           file, event, apc, apc_user, io_status, segments, length, offset, key);

    if (length % page_size) return STATUS_INVALID_PARAMETER;
    if (!io_status) return STATUS_ACCESS_VIOLATION;

    status = server_get_unix_fd( file, FILE_WRITE_DATA, &unix_handle,
                                 &needs_close, &type, &options );
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
                goto error;
            }
            status = FILE_GetNtStatus();
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

 error:
    if (needs_close) close( unix_handle );
    if (status == STATUS_SUCCESS)
    {
        io_status->u.Status = status;
        io_status->Information = total;
        TRACE("= SUCCESS (%u)\n", total);
        if (event) NtSetEvent( event, NULL );
        if (apc) NtQueueApcThread( GetCurrentThread(), (PNTAPCFUNC)apc,
                                   (ULONG_PTR)apc_user, (ULONG_PTR)io_status, 0 );
    }
    else
    {
        TRACE("= 0x%08x\n", status);
        if (status != STATUS_PENDING && event) NtResetEvent( event, NULL );
    }

    if (send_completion) NTDLL_AddCompletion( file, cvalue, status, total );

    return status;
}


struct async_ioctl
{
    HANDLE          handle;   /* handle to the device */
    HANDLE          event;    /* async event */
    void           *buffer;   /* buffer for output */
    ULONG           size;     /* size of buffer */
    PIO_APC_ROUTINE apc;      /* user apc params */
    void           *apc_arg;
};

/* callback for ioctl user APC */
static void WINAPI ioctl_apc( void *arg, IO_STATUS_BLOCK *io, ULONG reserved )
{
    struct async_ioctl *async = arg;
    if (async->apc) async->apc( async->apc_arg, io, reserved );
    RtlFreeHeap( GetProcessHeap(), 0, async );
}

/* callback for ioctl async I/O completion */
static NTSTATUS ioctl_completion( void *arg, IO_STATUS_BLOCK *io, NTSTATUS status, void **apc )
{
    struct async_ioctl *async = arg;

    if (status == STATUS_ALERTED)
    {
        SERVER_START_REQ( get_ioctl_result )
        {
            req->handle   = wine_server_obj_handle( async->handle );
            req->user_arg = wine_server_client_ptr( async );
            wine_server_set_reply( req, async->buffer, async->size );
            status = wine_server_call( req );
            if (status != STATUS_PENDING) io->Information = wine_server_reply_size( reply );
        }
        SERVER_END_REQ;
    }
    if (status != STATUS_PENDING)
    {
        io->u.Status = status;
        if (async->apc || async->event) *apc = ioctl_apc;
    }
    return status;
}

/* do an ioctl call through the server */
static NTSTATUS server_ioctl_file( HANDLE handle, HANDLE event,
                                   PIO_APC_ROUTINE apc, PVOID apc_context,
                                   IO_STATUS_BLOCK *io, ULONG code,
                                   const void *in_buffer, ULONG in_size,
                                   PVOID out_buffer, ULONG out_size )
{
    struct async_ioctl *async;
    NTSTATUS status;
    HANDLE wait_handle;
    ULONG options;
    ULONG_PTR cvalue = apc ? 0 : (ULONG_PTR)apc_context;

    if (!(async = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*async) )))
        return STATUS_NO_MEMORY;
    async->handle  = handle;
    async->event   = event;
    async->buffer  = out_buffer;
    async->size    = out_size;
    async->apc     = apc;
    async->apc_arg = apc_context;

    SERVER_START_REQ( ioctl )
    {
        req->code           = code;
        req->blocking       = !apc && !event && !cvalue;
        req->async.handle   = wine_server_obj_handle( handle );
        req->async.callback = wine_server_client_ptr( ioctl_completion );
        req->async.iosb     = wine_server_client_ptr( io );
        req->async.arg      = wine_server_client_ptr( async );
        req->async.event    = wine_server_obj_handle( event );
        req->async.cvalue   = cvalue;
        wine_server_add_data( req, in_buffer, in_size );
        wine_server_set_reply( req, out_buffer, out_size );
        status = wine_server_call( req );
        wait_handle = wine_server_ptr_handle( reply->wait );
        options     = reply->options;
        if (status != STATUS_PENDING) io->Information = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;

    if (status == STATUS_NOT_SUPPORTED)
        FIXME("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);

    if (status != STATUS_PENDING) RtlFreeHeap( GetProcessHeap(), 0, async );

    if (wait_handle)
    {
        NtWaitForSingleObject( wait_handle, (options & FILE_SYNCHRONOUS_IO_ALERT), NULL );
        status = io->u.Status;
        NtClose( wait_handle );
        RtlFreeHeap( GetProcessHeap(), 0, async );
    }

    return status;
}

/* Tell Valgrind to ignore any holes in structs we will be passing to the
 * server */
static void ignore_server_ioctl_struct_holes (ULONG code, const void *in_buffer,
                                              ULONG in_size)
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


/**************************************************************************
 *		NtDeviceIoControlFile			[NTDLL.@]
 *		ZwDeviceIoControlFile			[NTDLL.@]
 *
 * Perform an I/O control operation on an open file handle.
 *
 * PARAMS
 *  handle         [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  event          [I] Event to signal upon completion (or NULL)
 *  apc            [I] Callback to call upon completion (or NULL)
 *  apc_context    [I] Context for ApcRoutine (or NULL)
 *  io             [O] Receives information about the operation on return
 *  code           [I] Control code for the operation to perform
 *  in_buffer      [I] Source for any input data required (or NULL)
 *  in_size        [I] Size of InputBuffer
 *  out_buffer     [O] Source for any output data returned (or NULL)
 *  out_size       [I] Size of OutputBuffer
 *
 * RETURNS
 *  Success: 0. IoStatusBlock is updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtDeviceIoControlFile(HANDLE handle, HANDLE event,
                                      PIO_APC_ROUTINE apc, PVOID apc_context,
                                      PIO_STATUS_BLOCK io, ULONG code,
                                      PVOID in_buffer, ULONG in_size,
                                      PVOID out_buffer, ULONG out_size)
{
    ULONG device = (code >> 16);
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    TRACE("(%p,%p,%p,%p,%p,0x%08x,%p,0x%08x,%p,0x%08x)\n",
          handle, event, apc, apc_context, io, code,
          in_buffer, in_size, out_buffer, out_size);

    switch(device)
    {
    case FILE_DEVICE_DISK:
    case FILE_DEVICE_CD_ROM:
    case FILE_DEVICE_DVD:
    case FILE_DEVICE_CONTROLLER:
    case FILE_DEVICE_MASS_STORAGE:
        status = CDROM_DeviceIoControl(handle, event, apc, apc_context, io, code,
                                       in_buffer, in_size, out_buffer, out_size);
        break;
    case FILE_DEVICE_SERIAL_PORT:
        status = COMM_DeviceIoControl(handle, event, apc, apc_context, io, code,
                                      in_buffer, in_size, out_buffer, out_size);
        break;
    case FILE_DEVICE_TAPE:
        status = TAPE_DeviceIoControl(handle, event, apc, apc_context, io, code,
                                      in_buffer, in_size, out_buffer, out_size);
        break;
    }

    if (status == STATUS_NOT_SUPPORTED || status == STATUS_BAD_DEVICE_TYPE)
        status = server_ioctl_file( handle, event, apc, apc_context, io, code,
                                    in_buffer, in_size, out_buffer, out_size );

    if (status != STATUS_PENDING) io->u.Status = status;
    return status;
}


/**************************************************************************
 *              NtFsControlFile                 [NTDLL.@]
 *              ZwFsControlFile                 [NTDLL.@]
 *
 * Perform a file system control operation on an open file handle.
 *
 * PARAMS
 *  handle         [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  event          [I] Event to signal upon completion (or NULL)
 *  apc            [I] Callback to call upon completion (or NULL)
 *  apc_context    [I] Context for ApcRoutine (or NULL)
 *  io             [O] Receives information about the operation on return
 *  code           [I] Control code for the operation to perform
 *  in_buffer      [I] Source for any input data required (or NULL)
 *  in_size        [I] Size of InputBuffer
 *  out_buffer     [O] Source for any output data returned (or NULL)
 *  out_size       [I] Size of OutputBuffer
 *
 * RETURNS
 *  Success: 0. IoStatusBlock is updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtFsControlFile(HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                PVOID apc_context, PIO_STATUS_BLOCK io, ULONG code,
                                PVOID in_buffer, ULONG in_size, PVOID out_buffer, ULONG out_size)
{
    NTSTATUS status;

    TRACE("(%p,%p,%p,%p,%p,0x%08x,%p,0x%08x,%p,0x%08x)\n",
          handle, event, apc, apc_context, io, code,
          in_buffer, in_size, out_buffer, out_size);

    if (!io) return STATUS_INVALID_PARAMETER;

    ignore_server_ioctl_struct_holes( code, in_buffer, in_size );

    switch(code)
    {
    case FSCTL_DISMOUNT_VOLUME:
        status = server_ioctl_file( handle, event, apc, apc_context, io, code,
                                    in_buffer, in_size, out_buffer, out_size );
        if (!status) status = DIR_unmount_device( handle );
        break;

    case FSCTL_PIPE_PEEK:
        {
            FILE_PIPE_PEEK_BUFFER *buffer = out_buffer;
            int avail = 0, fd, needs_close;

            if (out_size < FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data ))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            if ((status = server_get_unix_fd( handle, FILE_READ_DATA, &fd, &needs_close, NULL, NULL )))
                break;

#ifdef FIONREAD
            if (ioctl( fd, FIONREAD, &avail ) != 0)
            {
                TRACE("FIONREAD failed reason: %s\n",strerror(errno));
                if (needs_close) close( fd );
                status = FILE_GetNtStatus();
                break;
            }
#endif
            if (!avail)  /* check for closed pipe */
            {
                struct pollfd pollfd;
                int ret;

                pollfd.fd = fd;
                pollfd.events = POLLIN;
                pollfd.revents = 0;
                ret = poll( &pollfd, 1, 0 );
                if (ret == -1 || (ret == 1 && (pollfd.revents & (POLLHUP|POLLERR))))
                {
                    if (needs_close) close( fd );
                    status = STATUS_PIPE_BROKEN;
                    break;
                }
            }
            buffer->NamedPipeState    = 0;  /* FIXME */
            buffer->ReadDataAvailable = avail;
            buffer->NumberOfMessages  = 0;  /* FIXME */
            buffer->MessageLength     = 0;  /* FIXME */
            io->Information = FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data );
            status = STATUS_SUCCESS;
            if (avail)
            {
                ULONG data_size = out_size - FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data );
                if (data_size)
                {
                    int res = recv( fd, buffer->Data, data_size, MSG_PEEK );
                    if (res >= 0) io->Information += res;
                }
            }
            if (needs_close) close( fd );
        }
        break;

    case FSCTL_PIPE_DISCONNECT:
        status = server_ioctl_file( handle, event, apc, apc_context, io, code,
                                    in_buffer, in_size, out_buffer, out_size );
        if (!status)
        {
            int fd = server_remove_fd_from_cache( handle );
            if (fd != -1) close( fd );
        }
        break;

    case FSCTL_PIPE_IMPERSONATE:
        FIXME("FSCTL_PIPE_IMPERSONATE: impersonating self\n");
        status = RtlImpersonateSelf( SecurityImpersonation );
        break;

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
    case FSCTL_PIPE_LISTEN:
    case FSCTL_PIPE_WAIT:
    default:
        status = server_ioctl_file( handle, event, apc, apc_context, io, code,
                                    in_buffer, in_size, out_buffer, out_size );
        break;
    }

    if (status != STATUS_PENDING) io->u.Status = status;
    return status;
}

/******************************************************************************
 *  NtSetVolumeInformationFile		[NTDLL.@]
 *  ZwSetVolumeInformationFile		[NTDLL.@]
 *
 * Set volume information for an open file handle.
 *
 * PARAMS
 *  FileHandle         [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  IoStatusBlock      [O] Receives information about the operation on return
 *  FsInformation      [I] Source for volume information
 *  Length             [I] Size of FsInformation
 *  FsInformationClass [I] Type of volume information to set
 *
 * RETURNS
 *  Success: 0. IoStatusBlock is updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtSetVolumeInformationFile(
	IN HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FsInformation,
        ULONG Length,
	FS_INFORMATION_CLASS FsInformationClass)
{
	FIXME("(%p,%p,%p,0x%08x,0x%08x) stub\n",
	FileHandle,IoStatusBlock,FsInformation,Length,FsInformationClass);
	return 0;
}

#if defined(__ANDROID__) && !defined(HAVE_FUTIMENS)
static int futimens( int fd, const struct timespec spec[2] )
{
    return syscall( __NR_utimensat, fd, NULL, spec, 0 );
}
#define UTIME_OMIT ((1 << 30) - 2)
#define HAVE_FUTIMENS
#endif  /* __ANDROID__ */

static NTSTATUS set_file_times( int fd, const LARGE_INTEGER *mtime, const LARGE_INTEGER *atime )
{
    NTSTATUS status = STATUS_SUCCESS;

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
    if (futimens( fd, tv ) == -1) status = FILE_GetNtStatus();

#elif defined(HAVE_FUTIMES) || defined(HAVE_FUTIMESAT)
    struct timeval tv[2];
    struct stat st;

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
    if (futimes( fd, tv ) == -1) status = FILE_GetNtStatus();
#elif defined(HAVE_FUTIMESAT)
    if (futimesat( fd, NULL, tv ) == -1) status = FILE_GetNtStatus();
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
    RtlSecondsSince1970ToTime( st->st_mtime, mtime );
    RtlSecondsSince1970ToTime( st->st_ctime, ctime );
    RtlSecondsSince1970ToTime( st->st_atime, atime );
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
    RtlSecondsSince1970ToTime( st->st_birthtime, creation );
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIM
    creation->QuadPart += st->st_birthtim.tv_nsec / 100;
#elif defined(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC)
    creation->QuadPart += st->st_birthtimespec.tv_nsec / 100;
#endif
#elif defined(HAVE_STRUCT_STAT___ST_BIRTHTIME)
    RtlSecondsSince1970ToTime( st->__st_birthtime, creation );
#ifdef HAVE_STRUCT_STAT___ST_BIRTHTIM
    creation->QuadPart += st->__st_birthtim.tv_nsec / 100;
#endif
#else
    *creation = *mtime;
#endif
}

/* fill in the file information that depends on the stat info */
NTSTATUS fill_stat_info( const struct stat *st, void *ptr, FILE_INFORMATION_CLASS class )
{
    switch (class)
    {
    case FileBasicInformation:
        {
            FILE_BASIC_INFORMATION *info = ptr;

            get_file_times( st, &info->LastWriteTime, &info->ChangeTime,
                            &info->LastAccessTime, &info->CreationTime );
            if (S_ISDIR(st->st_mode)) info->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            else info->FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
            if (!(st->st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
                info->FileAttributes |= FILE_ATTRIBUTE_READONLY;
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
            fill_stat_info( st, &info->BasicInformation, FileBasicInformation );
            fill_stat_info( st, &info->StandardInformation, FileStandardInformation );
            fill_stat_info( st, &info->InternalInformation, FileInternalInformation );
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
                info->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            }
            else
            {
                info->AllocationSize.QuadPart = (ULONGLONG)st->st_blocks * 512;
                info->EndOfFile.QuadPart      = st->st_size;
                info->FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
            }
            if (!(st->st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
                info->FileAttributes |= FILE_ATTRIBUTE_READONLY;
        }
        break;
    case FileIdFullDirectoryInformation:
        {
            FILE_ID_FULL_DIRECTORY_INFORMATION *info = ptr;
            info->FileId.QuadPart = st->st_ino;
            fill_stat_info( st, info, FileDirectoryInformation );
        }
        break;
    case FileIdBothDirectoryInformation:
        {
            FILE_ID_BOTH_DIRECTORY_INFORMATION *info = ptr;
            info->FileId.QuadPart = st->st_ino;
            fill_stat_info( st, info, FileDirectoryInformation );
        }
        break;

    default:
        return STATUS_INVALID_INFO_CLASS;
    }
    return STATUS_SUCCESS;
}

NTSTATUS server_get_unix_name( HANDLE handle, ANSI_STRING *unix_name )
{
    data_size_t size = 1024;
    NTSTATUS ret;
    char *name;

    for (;;)
    {
        name = RtlAllocateHeap( GetProcessHeap(), 0, size + 1 );
        if (!name) return STATUS_NO_MEMORY;
        unix_name->MaximumLength = size + 1;

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
            unix_name->Buffer = name;
            unix_name->Length = size;
            break;
        }
        RtlFreeHeap( GetProcessHeap(), 0, name );
        if (ret != STATUS_BUFFER_OVERFLOW) break;
    }
    return ret;
}

static NTSTATUS fill_name_info( const ANSI_STRING *unix_name, FILE_NAME_INFORMATION *info, LONG *name_len )
{
    UNICODE_STRING nt_name;
    NTSTATUS status;

    if (!(status = wine_unix_to_nt_file_name( unix_name, &nt_name )))
    {
        const WCHAR *ptr = nt_name.Buffer;
        const WCHAR *end = ptr + (nt_name.Length / sizeof(WCHAR));

        /* Skip the volume mount point. */
        while (ptr != end && *ptr == '\\') ++ptr;
        while (ptr != end && *ptr != '\\') ++ptr;
        while (ptr != end && *ptr == '\\') ++ptr;
        while (ptr != end && *ptr != '\\') ++ptr;

        info->FileNameLength = (end - ptr) * sizeof(WCHAR);
        if (*name_len < info->FileNameLength) status = STATUS_BUFFER_OVERFLOW;
        else *name_len = info->FileNameLength;

        memcpy( info->FileName, ptr, *name_len );
        RtlFreeUnicodeString( &nt_name );
    }

    return status;
}

/******************************************************************************
 *  NtQueryInformationFile		[NTDLL.@]
 *  ZwQueryInformationFile		[NTDLL.@]
 *
 * Get information about an open file handle.
 *
 * PARAMS
 *  hFile    [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  io       [O] Receives information about the operation on return
 *  ptr      [O] Destination for file information
 *  len      [I] Size of FileInformation
 *  class    [I] Type of file information to get
 *
 * RETURNS
 *  Success: 0. IoStatusBlock and FileInformation are updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtQueryInformationFile( HANDLE hFile, PIO_STATUS_BLOCK io,
                                        PVOID ptr, LONG len, FILE_INFORMATION_CLASS class )
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
        sizeof(FILE_ACCESS_INFORMATION),               /* FileAccessInformation */
        sizeof(FILE_NAME_INFORMATION),                 /* FileNameInformation */
        sizeof(FILE_RENAME_INFORMATION)-sizeof(WCHAR), /* FileRenameInformation */
        0,                                             /* FileLinkInformation */
        sizeof(FILE_NAMES_INFORMATION)-sizeof(WCHAR),  /* FileNamesInformation */
        sizeof(FILE_DISPOSITION_INFORMATION),          /* FileDispositionInformation */
        sizeof(FILE_POSITION_INFORMATION),             /* FilePositionInformation */
        sizeof(FILE_FULL_EA_INFORMATION),              /* FileFullEaInformation */
        sizeof(FILE_MODE_INFORMATION),                 /* FileModeInformation */
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
        0,                                             /* FileNetworkOpenInformation */
        0,                                             /* FileAttributeTagInformation */
        0,                                             /* FileTrackingInformation */
        0,                                             /* FileIdBothDirectoryInformation */
        0,                                             /* FileIdFullDirectoryInformation */
        0,                                             /* FileValidDataLengthInformation */
        0,                                             /* FileShortNameInformation */
        0,
        0,
        0,
        0,                                             /* FileSfioReserveInformation */
        0,                                             /* FileSfioVolumeInformation */
        0,                                             /* FileHardLinkInformation */
        0,
        0,                                             /* FileNormalizedNameInformation */
        0,
        0,                                             /* FileIdGlobalTxDirectoryInformation */
        0,
        0,
        0,
        0                                              /* FileStandardLinkInformation */
    };

    struct stat st;
    int fd, needs_close = FALSE;

    TRACE("(%p,%p,%p,0x%08x,0x%08x)\n", hFile, io, ptr, len, class);

    io->Information = 0;

    if (class <= 0 || class >= FileMaximumInformation)
        return io->u.Status = STATUS_INVALID_INFO_CLASS;
    if (!info_sizes[class])
    {
        FIXME("Unsupported class (%d)\n", class);
        return io->u.Status = STATUS_NOT_IMPLEMENTED;
    }
    if (len < info_sizes[class])
        return io->u.Status = STATUS_INFO_LENGTH_MISMATCH;

    if (class != FilePipeInformation && class != FilePipeLocalInformation)
    {
        if ((io->u.Status = server_get_unix_fd( hFile, 0, &fd, &needs_close, NULL, NULL )))
            return io->u.Status;
    }

    switch (class)
    {
    case FileBasicInformation:
        if (fstat( fd, &st ) == -1)
            io->u.Status = FILE_GetNtStatus();
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            io->u.Status = STATUS_INVALID_INFO_CLASS;
        else
            fill_stat_info( &st, ptr, class );
        break;
    case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION *info = ptr;

            if (fstat( fd, &st ) == -1) io->u.Status = FILE_GetNtStatus();
            else
            {
                fill_stat_info( &st, info, class );
                info->DeletePending = FALSE; /* FIXME */
            }
        }
        break;
    case FilePositionInformation:
        {
            FILE_POSITION_INFORMATION *info = ptr;
            off_t res = lseek( fd, 0, SEEK_CUR );
            if (res == (off_t)-1) io->u.Status = FILE_GetNtStatus();
            else info->CurrentByteOffset.QuadPart = res;
        }
        break;
    case FileInternalInformation:
        if (fstat( fd, &st ) == -1) io->u.Status = FILE_GetNtStatus();
        else fill_stat_info( &st, ptr, class );
        break;
    case FileEaInformation:
        {
            FILE_EA_INFORMATION *info = ptr;
            info->EaSize = 0;
        }
        break;
    case FileEndOfFileInformation:
        if (fstat( fd, &st ) == -1) io->u.Status = FILE_GetNtStatus();
        else fill_stat_info( &st, ptr, class );
        break;
    case FileAllInformation:
        {
            FILE_ALL_INFORMATION *info = ptr;
            ANSI_STRING unix_name;

            if (fstat( fd, &st ) == -1) io->u.Status = FILE_GetNtStatus();
            else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
                io->u.Status = STATUS_INVALID_INFO_CLASS;
            else if (!(io->u.Status = server_get_unix_name( hFile, &unix_name )))
            {
                LONG name_len = len - FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName);

                fill_stat_info( &st, info, FileAllInformation );
                info->StandardInformation.DeletePending = FALSE; /* FIXME */
                info->EaInformation.EaSize = 0;
                info->AccessInformation.AccessFlags = 0;  /* FIXME */
                info->PositionInformation.CurrentByteOffset.QuadPart = lseek( fd, 0, SEEK_CUR );
                info->ModeInformation.Mode = 0;  /* FIXME */
                info->AlignmentInformation.AlignmentRequirement = 1;  /* FIXME */

                io->u.Status = fill_name_info( &unix_name, &info->NameInformation, &name_len );
                RtlFreeAnsiString( &unix_name );
                io->Information = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + name_len;
            }
        }
        break;
    case FileMailslotQueryInformation:
        {
            FILE_MAILSLOT_QUERY_INFORMATION *info = ptr;

            SERVER_START_REQ( set_mailslot_info )
            {
                req->handle = wine_server_obj_handle( hFile );
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
                if ((tmpbuf = RtlAllocateHeap( GetProcessHeap(), 0, size )))
                {
                    if (!server_get_unix_fd( hFile, FILE_READ_DATA, &fd, &needs_close, NULL, NULL ))
                    {
                        int res = recv( fd, tmpbuf, size, MSG_PEEK );
                        info->MessagesAvailable = (res > 0);
                        info->NextMessageSize = (res >= 0) ? res : MAILSLOT_NO_MESSAGE;
                        if (needs_close) close( fd );
                    }
                    RtlFreeHeap( GetProcessHeap(), 0, tmpbuf );
                }
            }
        }
        break;
    case FilePipeInformation:
        {
            FILE_PIPE_INFORMATION* pi = ptr;

            SERVER_START_REQ( get_named_pipe_info )
            {
                req->handle = wine_server_obj_handle( hFile );
                if (!(io->u.Status = wine_server_call( req )))
                {
                    pi->ReadMode       = (reply->flags & NAMED_PIPE_MESSAGE_STREAM_READ) ?
                        FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE;
                    pi->CompletionMode = (reply->flags & NAMED_PIPE_NONBLOCKING_MODE) ?
                        FILE_PIPE_COMPLETE_OPERATION : FILE_PIPE_QUEUE_OPERATION;
                }
            }
            SERVER_END_REQ;
        }
        break;
    case FilePipeLocalInformation:
        {
            FILE_PIPE_LOCAL_INFORMATION* pli = ptr;

            SERVER_START_REQ( get_named_pipe_info )
            {
                req->handle = wine_server_obj_handle( hFile );
                if (!(io->u.Status = wine_server_call( req )))
                {
                    pli->NamedPipeType = (reply->flags & NAMED_PIPE_MESSAGE_STREAM_WRITE) ? 
                        FILE_PIPE_TYPE_MESSAGE : FILE_PIPE_TYPE_BYTE;
                    switch (reply->sharing)
                    {
                        case FILE_SHARE_READ:
                            pli->NamedPipeConfiguration = FILE_PIPE_OUTBOUND;
                            break;
                        case FILE_SHARE_WRITE:
                            pli->NamedPipeConfiguration = FILE_PIPE_INBOUND;
                            break;
                        case FILE_SHARE_READ | FILE_SHARE_WRITE:
                            pli->NamedPipeConfiguration = FILE_PIPE_FULL_DUPLEX;
                            break;
                    }
                    pli->MaximumInstances = reply->maxinstances;
                    pli->CurrentInstances = reply->instances;
                    pli->InboundQuota = reply->insize;
                    pli->ReadDataAvailable = 0; /* FIXME */
                    pli->OutboundQuota = reply->outsize;
                    pli->WriteQuotaAvailable = 0; /* FIXME */
                    pli->NamedPipeState = 0; /* FIXME */
                    pli->NamedPipeEnd = (reply->flags & NAMED_PIPE_SERVER_END) ?
                        FILE_PIPE_SERVER_END : FILE_PIPE_CLIENT_END;
                }
            }
            SERVER_END_REQ;
        }
        break;
    case FileNameInformation:
        {
            FILE_NAME_INFORMATION *info = ptr;
            ANSI_STRING unix_name;

            if (!(io->u.Status = server_get_unix_name( hFile, &unix_name )))
            {
                LONG name_len = len - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
                io->u.Status = fill_name_info( &unix_name, info, &name_len );
                RtlFreeAnsiString( &unix_name );
                io->Information = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) + name_len;
            }
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
 *  NtSetInformationFile		[NTDLL.@]
 *  ZwSetInformationFile		[NTDLL.@]
 *
 * Set information about an open file handle.
 *
 * PARAMS
 *  handle  [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  io      [O] Receives information about the operation on return
 *  ptr     [I] Source for file information
 *  len     [I] Size of FileInformation
 *  class   [I] Type of file information to set
 *
 * RETURNS
 *  Success: 0. io is updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtSetInformationFile(HANDLE handle, PIO_STATUS_BLOCK io,
                                     PVOID ptr, ULONG len, FILE_INFORMATION_CLASS class)
{
    int fd, needs_close;

    TRACE("(%p,%p,%p,0x%08x,0x%08x)\n", handle, io, ptr, len, class);

    io->u.Status = STATUS_SUCCESS;
    switch (class)
    {
    case FileBasicInformation:
        if (len >= sizeof(FILE_BASIC_INFORMATION))
        {
            struct stat st;
            const FILE_BASIC_INFORMATION *info = ptr;

            if ((io->u.Status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )))
                return io->u.Status;

            if (info->LastAccessTime.QuadPart || info->LastWriteTime.QuadPart)
                io->u.Status = set_file_times( fd, &info->LastWriteTime, &info->LastAccessTime );

            if (io->u.Status == STATUS_SUCCESS && info->FileAttributes)
            {
                if (fstat( fd, &st ) == -1) io->u.Status = FILE_GetNtStatus();
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
                        st.st_mode |= (0600 | ((st.st_mode & 044) >> 1)) & (~FILE_umask);
                    }
                    if (fchmod( fd, st.st_mode ) == -1) io->u.Status = FILE_GetNtStatus();
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
                io->u.Status = FILE_GetNtStatus();

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
            io->u.Status = FILE_GetNtStatus();

            if (needs_close) close( fd );
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

            if (fstat( fd, &st ) == -1) io->u.Status = FILE_GetNtStatus();
            else if (info->ValidDataLength.QuadPart <= 0 || (off_t)info->ValidDataLength.QuadPart > st.st_size)
                io->u.Status = STATUS_INVALID_PARAMETER;
            else
            {
#ifdef HAVE_FALLOCATE
                if (fallocate( fd, 0, 0, (off_t)info->ValidDataLength.QuadPart ) == -1)
                {
                    NTSTATUS status = FILE_GetNtStatus();
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

    default:
        FIXME("Unsupported class (%d)\n", class);
        io->u.Status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    io->Information = 0;
    return io->u.Status;
}


/******************************************************************************
 *              NtQueryFullAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryFullAttributesFile( const OBJECT_ATTRIBUTES *attr,
                                           FILE_NETWORK_OPEN_INFORMATION *info )
{
    ANSI_STRING unix_name;
    NTSTATUS status;

    if (!(status = nt_to_unix_file_name_attr( attr, &unix_name, FILE_OPEN )))
    {
        struct stat st;

        if (stat( unix_name.Buffer, &st ) == -1)
            status = FILE_GetNtStatus();
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
        {
            FILE_BASIC_INFORMATION basic;
            FILE_STANDARD_INFORMATION std;

            fill_stat_info( &st, &basic, FileBasicInformation );
            fill_stat_info( &st, &std, FileStandardInformation );

            info->CreationTime   = basic.CreationTime;
            info->LastAccessTime = basic.LastAccessTime;
            info->LastWriteTime  = basic.LastWriteTime;
            info->ChangeTime     = basic.ChangeTime;
            info->AllocationSize = std.AllocationSize;
            info->EndOfFile      = std.EndOfFile;
            info->FileAttributes = basic.FileAttributes;
            if (DIR_is_hidden_file( attr->ObjectName ))
                info->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        }
        RtlFreeAnsiString( &unix_name );
    }
    else WARN("%s not found (%x)\n", debugstr_us(attr->ObjectName), status );
    return status;
}


/******************************************************************************
 *              NtQueryAttributesFile   (NTDLL.@)
 *              ZwQueryAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryAttributesFile( const OBJECT_ATTRIBUTES *attr, FILE_BASIC_INFORMATION *info )
{
    ANSI_STRING unix_name;
    NTSTATUS status;

    if (!(status = nt_to_unix_file_name_attr( attr, &unix_name, FILE_OPEN )))
    {
        struct stat st;

        if (stat( unix_name.Buffer, &st ) == -1)
            status = FILE_GetNtStatus();
        else if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode))
            status = STATUS_INVALID_INFO_CLASS;
        else
        {
            status = fill_stat_info( &st, info, FileBasicInformation );
            if (DIR_is_hidden_file( attr->ObjectName ))
                info->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        }
        RtlFreeAnsiString( &unix_name );
    }
    else WARN("%s not found (%x)\n", debugstr_us(attr->ObjectName), status );
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

/******************************************************************************
 *              get_device_info
 *
 * Implementation of the FileFsDeviceInformation query for NtQueryVolumeInformationFile.
 */
static NTSTATUS get_device_info( int fd, FILE_FS_DEVICE_INFORMATION *info )
{
    struct stat st;

    info->Characteristics = 0;
    if (fstat( fd, &st ) < 0) return FILE_GetNtStatus();
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
        case 0x517B:  /* smbfs */
        case 0x564c:  /* ncpfs */
            info->DeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
            info->Characteristics |= FILE_REMOTE_DEVICE;
            break;
        case 0x01021994:  /* tmpfs */
        case 0x28cd3d45:  /* cramfs */
        case 0x1373:      /* devfs */
        case 0x9fa0:      /* procfs */
            info->DeviceType = FILE_DEVICE_VIRTUAL_DISK;
            break;
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
                info->DeviceType = FILE_DEVICE_VIRTUAL_DISK;
                break;
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
 *  NtQueryVolumeInformationFile		[NTDLL.@]
 *  ZwQueryVolumeInformationFile		[NTDLL.@]
 *
 * Get volume information for an open file handle.
 *
 * PARAMS
 *  handle      [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  io          [O] Receives information about the operation on return
 *  buffer      [O] Destination for volume information
 *  length      [I] Size of FsInformation
 *  info_class  [I] Type of volume information to set
 *
 * RETURNS
 *  Success: 0. io and buffer are updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtQueryVolumeInformationFile( HANDLE handle, PIO_STATUS_BLOCK io,
                                              PVOID buffer, ULONG length,
                                              FS_INFORMATION_CLASS info_class )
{
    int fd, needs_close;
    struct stat st;
    static int once;

    if ((io->u.Status = server_get_unix_fd( handle, 0, &fd, &needs_close, NULL, NULL )) != STATUS_SUCCESS)
        return io->u.Status;

    io->u.Status = STATUS_NOT_IMPLEMENTED;
    io->Information = 0;

    switch( info_class )
    {
    case FileFsVolumeInformation:
        if (!once++) FIXME( "%p: volume info not supported\n", handle );
        break;
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
                io->u.Status = FILE_GetNtStatus();
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
                    io->u.Status = FILE_GetNtStatus();
                    break;
                }
                bsize = stfs.f_frsize;
#else
                struct statfs stfs;
                if (fstatfs( fd, &stfs ) < 0)
                {
                    io->u.Status = FILE_GetNtStatus();
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
        if (length < offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[sizeof(ntfsW)/sizeof(WCHAR)] ))
            io->u.Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            FILE_FS_ATTRIBUTE_INFORMATION *info = buffer;

            FIXME( "%p: faking attribute info\n", handle );
            info->FileSystemAttribute = FILE_SUPPORTS_ENCRYPTION | FILE_FILE_COMPRESSION |
                                        FILE_PERSISTENT_ACLS | FILE_UNICODE_ON_DISK |
                                        FILE_CASE_PRESERVED_NAMES | FILE_CASE_SENSITIVE_SEARCH;
            info->MaximumComponentNameLength = MAXIMUM_FILENAME_LENGTH - 1;
            info->FileSystemNameLength = sizeof(ntfsW);
            memcpy(info->FileSystemName, ntfsW, sizeof(ntfsW));

            io->Information = sizeof(*info);
            io->u.Status = STATUS_SUCCESS;
        }
        break;
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


/******************************************************************
 *		NtQueryEaFile  (NTDLL.@)
 *
 * Read extended attributes from NTFS files.
 *
 * PARAMS
 *  hFile         [I] File handle, must be opened with FILE_READ_EA access
 *  iosb          [O] Receives information about the operation on return
 *  buffer        [O] Output buffer
 *  length        [I] Length of output buffer
 *  single_entry  [I] Only read and return one entry
 *  ea_list       [I] Optional list with names of EAs to return
 *  ea_list_len   [I] Length of ea_list in bytes
 *  ea_index      [I] Optional pointer to 1-based index of attribute to return
 *  restart       [I] restart EA scan
 *
 * RETURNS
 *  Success: 0. Atrributes read into buffer
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtQueryEaFile( HANDLE hFile, PIO_STATUS_BLOCK iosb, PVOID buffer, ULONG length,
                               BOOLEAN single_entry, PVOID ea_list, ULONG ea_list_len,
                               PULONG ea_index, BOOLEAN restart )
{
    FIXME("(%p,%p,%p,%d,%d,%p,%d,%p,%d) stub\n",
            hFile, iosb, buffer, length, single_entry, ea_list,
            ea_list_len, ea_index, restart);
    return STATUS_ACCESS_DENIED;
}


/******************************************************************
 *		NtSetEaFile  (NTDLL.@)
 *
 * Update extended attributes for NTFS files.
 *
 * PARAMS
 *  hFile         [I] File handle, must be opened with FILE_READ_EA access
 *  iosb          [O] Receives information about the operation on return
 *  buffer        [I] Buffer with EA information
 *  length        [I] Length of buffer
 *
 * RETURNS
 *  Success: 0. Attributes are updated
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtSetEaFile( HANDLE hFile, PIO_STATUS_BLOCK iosb, PVOID buffer, ULONG length )
{
    FIXME("(%p,%p,%p,%d) stub\n", hFile, iosb, buffer, length);
    return STATUS_ACCESS_DENIED;
}


/******************************************************************
 *		NtFlushBuffersFile  (NTDLL.@)
 *
 * Flush any buffered data on an open file handle.
 *
 * PARAMS
 *  FileHandle         [I] Handle returned from ZwOpenFile() or ZwCreateFile()
 *  IoStatusBlock      [O] Receives information about the operation on return
 *
 * RETURNS
 *  Success: 0. IoStatusBlock is updated.
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtFlushBuffersFile( HANDLE hFile, IO_STATUS_BLOCK* IoStatusBlock )
{
    NTSTATUS ret;
    HANDLE hEvent = NULL;
    enum server_fd_type type;
    int fd, needs_close;

    ret = server_get_unix_fd( hFile, FILE_WRITE_DATA, &fd, &needs_close, &type, NULL );

    if (!ret && type == FD_TYPE_SERIAL)
    {
        ret = COMM_FlushBuffersFile( fd );
    }
    else
    {
        SERVER_START_REQ( flush_file )
        {
            req->handle = wine_server_obj_handle( hFile );
            ret = wine_server_call( req );
            hEvent = wine_server_ptr_handle( reply->event );
        }
        SERVER_END_REQ;
        if (!ret && hEvent)
        {
            ret = NtWaitForSingleObject( hEvent, FALSE, NULL );
            NtClose( hEvent );
        }
    }

    if (needs_close) close( fd );
    return ret;
}

/******************************************************************
 *		NtLockFile       (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtLockFile( HANDLE hFile, HANDLE lock_granted_event,
                            PIO_APC_ROUTINE apc, void* apc_user,
                            PIO_STATUS_BLOCK io_status, PLARGE_INTEGER offset,
                            PLARGE_INTEGER count, ULONG* key, BOOLEAN dont_wait,
                            BOOLEAN exclusive )
{
    NTSTATUS    ret;
    HANDLE      handle;
    BOOLEAN     async;
    static BOOLEAN     warn = TRUE;

    if (apc || io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if (apc_user && warn)
    {
        FIXME("I/O completion on lock not implemented yet\n");
        warn = FALSE;
    }

    for (;;)
    {
        SERVER_START_REQ( lock_file )
        {
            req->handle      = wine_server_obj_handle( hFile );
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
            if (!ret && lock_granted_event) NtSetEvent(lock_granted_event, NULL);
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
        else
        {
            LARGE_INTEGER time;
    
            /* Unix lock conflict, sleep a bit and retry */
            time.QuadPart = 100 * (ULONGLONG)10000;
            time.QuadPart = -time.QuadPart;
            NtDelayExecution( FALSE, &time );
        }
    }
}


/******************************************************************
 *		NtUnlockFile    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtUnlockFile( HANDLE hFile, PIO_STATUS_BLOCK io_status,
                              PLARGE_INTEGER offset, PLARGE_INTEGER count,
                              PULONG key )
{
    NTSTATUS status;

    TRACE( "%p %x%08x %x%08x\n",
           hFile, offset->u.HighPart, offset->u.LowPart, count->u.HighPart, count->u.LowPart );

    if (io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    SERVER_START_REQ( unlock_file )
    {
        req->handle = wine_server_obj_handle( hFile );
        req->offset = offset->QuadPart;
        req->count  = count->QuadPart;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *		NtCreateNamedPipeFile    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtCreateNamedPipeFile( PHANDLE handle, ULONG access,
                                       POBJECT_ATTRIBUTES attr, PIO_STATUS_BLOCK iosb,
                                       ULONG sharing, ULONG dispo, ULONG options,
                                       ULONG pipe_type, ULONG read_mode, 
                                       ULONG completion_mode, ULONG max_inst,
                                       ULONG inbound_quota, ULONG outbound_quota,
                                       PLARGE_INTEGER timeout)
{
    NTSTATUS    status;

    TRACE("(%p %x %s %p %x %d %x %d %d %d %d %d %d %p)\n",
          handle, access, debugstr_w(attr->ObjectName->Buffer), iosb, sharing, dispo,
          options, pipe_type, read_mode, completion_mode, max_inst, inbound_quota,
          outbound_quota, timeout);

    /* assume we only get relative timeout */
    if (timeout->QuadPart > 0)
        FIXME("Wrong time %s\n", wine_dbgstr_longlong(timeout->QuadPart));

    SERVER_START_REQ( create_named_pipe )
    {
        req->access  = access;
        req->attributes = attr->Attributes;
        req->rootdir = wine_server_obj_handle( attr->RootDirectory );
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
        wine_server_add_data( req, attr->ObjectName->Buffer,
                              attr->ObjectName->Length );
        status = wine_server_call( req );
        if (!status) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *		NtDeleteFile    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtDeleteFile( POBJECT_ATTRIBUTES ObjectAttributes )
{
    NTSTATUS status;
    HANDLE hFile;
    IO_STATUS_BLOCK io;

    TRACE("%p\n", ObjectAttributes);
    status = NtCreateFile( &hFile, GENERIC_READ | GENERIC_WRITE | DELETE,
                           ObjectAttributes, &io, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
                           FILE_OPEN, FILE_DELETE_ON_CLOSE, NULL, 0 );
    if (status == STATUS_SUCCESS) status = NtClose(hFile);
    return status;
}

/******************************************************************
 *		NtCancelIoFileEx    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtCancelIoFileEx( HANDLE hFile, PIO_STATUS_BLOCK iosb, PIO_STATUS_BLOCK io_status )
{
    LARGE_INTEGER timeout;

    TRACE("%p %p %p\n", hFile, iosb, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( hFile );
        req->iosb        = wine_server_client_ptr( iosb );
        req->only_thread = FALSE;
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (io_status->u.Status)
        return io_status->u.Status;

    /* Let some APC be run, so that we can run the remaining APCs on hFile
     * either the cancelation of the pending one, but also the execution
     * of the queued APC, but not yet run. This is needed to ensure proper
     * clean-up of allocated data.
     */
    timeout.QuadPart = 0;
    NtDelayExecution( TRUE, &timeout );
    return io_status->u.Status;
}

/******************************************************************
 *		NtCancelIoFile    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtCancelIoFile( HANDLE hFile, PIO_STATUS_BLOCK io_status )
{
    LARGE_INTEGER timeout;

    TRACE("%p %p\n", hFile, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( hFile );
        req->iosb        = 0;
        req->only_thread = TRUE;
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (io_status->u.Status)
        return io_status->u.Status;

    /* Let some APC be run, so that we can run the remaining APCs on hFile
     * either the cancelation of the pending one, but also the execution
     * of the queued APC, but not yet run. This is needed to ensure proper
     * clean-up of allocated data.
     */
    timeout.QuadPart = 0;
    NtDelayExecution( TRUE, &timeout );
    return io_status->u.Status;
}

/******************************************************************************
 *  NtCreateMailslotFile	[NTDLL.@]
 *  ZwCreateMailslotFile	[NTDLL.@]
 *
 * PARAMS
 *  pHandle          [O] pointer to receive the handle created
 *  DesiredAccess    [I] access mode (read, write, etc)
 *  ObjectAttributes [I] fully qualified NT path of the mailslot
 *  IoStatusBlock    [O] receives completion status and other info
 *  CreateOptions    [I]
 *  MailslotQuota    [I]
 *  MaxMessageSize   [I]
 *  TimeOut          [I]
 *
 * RETURNS
 *  An NT status code
 */
NTSTATUS WINAPI NtCreateMailslotFile(PHANDLE pHandle, ULONG DesiredAccess,
     POBJECT_ATTRIBUTES attr, PIO_STATUS_BLOCK IoStatusBlock,
     ULONG CreateOptions, ULONG MailslotQuota, ULONG MaxMessageSize,
     PLARGE_INTEGER TimeOut)
{
    LARGE_INTEGER timeout;
    NTSTATUS ret;

    TRACE("%p %08x %p %p %08x %08x %08x %p\n",
              pHandle, DesiredAccess, attr, IoStatusBlock,
              CreateOptions, MailslotQuota, MaxMessageSize, TimeOut);

    if (!pHandle) return STATUS_ACCESS_VIOLATION;
    if (!attr) return STATUS_INVALID_PARAMETER;
    if (!attr->ObjectName) return STATUS_OBJECT_PATH_SYNTAX_BAD;

    /*
     *  For a NULL TimeOut pointer set the default timeout value
     */
    if  (!TimeOut)
        timeout.QuadPart = -1;
    else
        timeout.QuadPart = TimeOut->QuadPart;

    SERVER_START_REQ( create_mailslot )
    {
        req->access = DesiredAccess;
        req->attributes = attr->Attributes;
        req->rootdir = wine_server_obj_handle( attr->RootDirectory );
        req->max_msgsize = MaxMessageSize;
        req->read_timeout = timeout.QuadPart;
        wine_server_add_data( req, attr->ObjectName->Buffer,
                              attr->ObjectName->Length );
        ret = wine_server_call( req );
        if( ret == STATUS_SUCCESS )
            *pHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
 
    return ret;
}
