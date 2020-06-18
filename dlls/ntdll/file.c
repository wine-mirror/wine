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
#ifdef MAJOR_IN_MKDEV
# include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
# include <sys/sysmacros.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#ifdef HAVE_SYS_VFS_H
/* Work around a conflict with Solaris' system list defined in sys/list.h. */
#define list SYSLIST
#define list_next SYSLIST_NEXT
#define list_prev SYSLIST_PREV
#define list_head SYSLIST_HEAD
#define list_tail SYSLIST_TAIL
#define list_move_tail SYSLIST_MOVE_TAIL
#define list_remove SYSLIST_REMOVE
# include <sys/vfs.h>
#undef list
#undef list_next
#undef list_prev
#undef list_head
#undef list_tail
#undef list_move_tail
#undef list_remove
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"

#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddk.h"
#include "ddk/ntddser.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


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
    return unix_funcs->NtOpenFile( handle, access, attr, io, sharing, options );
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
    return unix_funcs->NtCreateFile( handle, access, attr, io, alloc_size, attributes,
                                     sharing, disposition, options, ea_buffer, ea_length );
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
    char*               buffer;
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
        RtlFreeHeap( GetProcessHeap(), 0, io );
        io = next;
    }

    if ((io = RtlAllocateHeap( GetProcessHeap(), 0, size )))
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
    return unix_funcs->NtReadFile( hFile, hEvent, apc, apc_user, io_status, buffer, length, offset, key );
}


/******************************************************************************
 *  NtReadFileScatter   [NTDLL.@]
 *  ZwReadFileScatter   [NTDLL.@]
 */
NTSTATUS WINAPI NtReadFileScatter( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                   PIO_STATUS_BLOCK io_status, FILE_SEGMENT_ELEMENT *segments,
                                   ULONG length, PLARGE_INTEGER offset, PULONG key )
{
    return unix_funcs->NtReadFileScatter( file, event, apc, apc_user, io_status,
                                          segments, length, offset, key );
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
    return unix_funcs->NtWriteFile( hFile, hEvent, apc, apc_user, io_status, buffer, length, offset, key );
}


/******************************************************************************
 *  NtWriteFileGather   [NTDLL.@]
 *  ZwWriteFileGather   [NTDLL.@]
 */
NTSTATUS WINAPI NtWriteFileGather( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                   PIO_STATUS_BLOCK io_status, FILE_SEGMENT_ELEMENT *segments,
                                   ULONG length, PLARGE_INTEGER offset, PULONG key )
{
    return unix_funcs->NtWriteFileGather( file, event, apc, apc_user, io_status,
                                          segments, length, offset, key );
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
    return unix_funcs->NtDeviceIoControlFile( handle, event, apc, apc_context, io, code,
                                              in_buffer, in_size, out_buffer, out_size );
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
    return unix_funcs->NtFsControlFile( handle, event, apc, apc_context, io, code,
                                        in_buffer, in_size, out_buffer, out_size );
}


struct read_changes_fileio
{
    struct async_fileio io;
    void               *buffer;
    ULONG               buffer_size;
    ULONG               data_size;
    char                data[1];
};

static NTSTATUS read_changes_apc( void *user, IO_STATUS_BLOCK *iosb, NTSTATUS status )
{
    struct read_changes_fileio *fileio = user;
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
 *  NtNotifyChangeDirectoryFile [NTDLL.@]
 */
NTSTATUS WINAPI NtNotifyChangeDirectoryFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                             void *apc_context, PIO_STATUS_BLOCK iosb, void *buffer,
                                             ULONG buffer_size, ULONG filter, BOOLEAN subtree )
{
    struct read_changes_fileio *fileio;
    NTSTATUS status;
    ULONG size = max( 4096, buffer_size );

    TRACE( "%p %p %p %p %p %p %u %u %d\n",
           handle, event, apc, apc_context, iosb, buffer, buffer_size, filter, subtree );

    if (!iosb) return STATUS_ACCESS_VIOLATION;
    if (filter == 0 || (filter & ~FILE_NOTIFY_ALL)) return STATUS_INVALID_PARAMETER;

    fileio = (struct read_changes_fileio *)alloc_fileio( offsetof(struct read_changes_fileio, data[size]),
                                                         read_changes_apc, handle );
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

    if (status != STATUS_PENDING) RtlFreeHeap( GetProcessHeap(), 0, fileio );
    return status;
}

/******************************************************************************
 *  NtSetVolumeInformationFile		[NTDLL.@]
 *  ZwSetVolumeInformationFile		[NTDLL.@]
 */
NTSTATUS WINAPI NtSetVolumeInformationFile( HANDLE handle, IO_STATUS_BLOCK *io, void *info,
                                            ULONG length, FS_INFORMATION_CLASS class )
{
    return unix_funcs->NtSetVolumeInformationFile( handle, io, info, length, class );
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
    return unix_funcs->NtQueryInformationFile( hFile, io, ptr, len, class );
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
    return unix_funcs->NtSetInformationFile( handle, io, ptr, len, class );
}


/******************************************************************************
 *              NtQueryFullAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryFullAttributesFile( const OBJECT_ATTRIBUTES *attr,
                                           FILE_NETWORK_OPEN_INFORMATION *info )
{
    return unix_funcs->NtQueryFullAttributesFile( attr, info );
}


/******************************************************************************
 *              NtQueryAttributesFile   (NTDLL.@)
 *              ZwQueryAttributesFile   (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryAttributesFile( const OBJECT_ATTRIBUTES *attr, FILE_BASIC_INFORMATION *info )
{
    return unix_funcs->NtQueryAttributesFile( attr, info );
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
    return unix_funcs->NtQueryVolumeInformationFile( handle, io, buffer, length, info_class );
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
 *  Success: 0. Attributes read into buffer
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
NTSTATUS WINAPI NtFlushBuffersFile( HANDLE hFile, IO_STATUS_BLOCK *io )
{
    return unix_funcs->NtFlushBuffersFile( hFile, io );
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
 */
NTSTATUS WINAPI NtCreateNamedPipeFile( PHANDLE handle, ULONG access,
                                       POBJECT_ATTRIBUTES attr, PIO_STATUS_BLOCK iosb,
                                       ULONG sharing, ULONG dispo, ULONG options,
                                       ULONG pipe_type, ULONG read_mode,
                                       ULONG completion_mode, ULONG max_inst,
                                       ULONG inbound_quota, ULONG outbound_quota,
                                       PLARGE_INTEGER timeout)
{
    return unix_funcs->NtCreateNamedPipeFile( handle, access, attr, iosb, sharing, dispo, options,
                                              pipe_type, read_mode, completion_mode, max_inst,
                                              inbound_quota, outbound_quota, timeout );
}

/******************************************************************
 *		NtDeleteFile    (NTDLL.@)
 */
NTSTATUS WINAPI NtDeleteFile( OBJECT_ATTRIBUTES *attr )
{
    return unix_funcs->NtDeleteFile( attr );
}

/******************************************************************
 *		NtCancelIoFileEx    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtCancelIoFileEx( HANDLE hFile, PIO_STATUS_BLOCK iosb, PIO_STATUS_BLOCK io_status )
{
    TRACE("%p %p %p\n", hFile, iosb, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( hFile );
        req->iosb        = wine_server_client_ptr( iosb );
        req->only_thread = FALSE;
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;

    return io_status->u.Status;
}

/******************************************************************
 *		NtCancelIoFile    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtCancelIoFile( HANDLE hFile, PIO_STATUS_BLOCK io_status )
{
    TRACE("%p %p\n", hFile, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( hFile );
        req->iosb        = 0;
        req->only_thread = TRUE;
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;

    return io_status->u.Status;
}

/******************************************************************************
 *  NtCreateMailslotFile	[NTDLL.@]
 *  ZwCreateMailslotFile	[NTDLL.@]
 */
NTSTATUS WINAPI NtCreateMailslotFile( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                      IO_STATUS_BLOCK *io, ULONG options, ULONG quota, ULONG msg_size,
                                      LARGE_INTEGER *timeout )
{
    return unix_funcs->NtCreateMailslotFile( handle, access, attr, io, options, quota, msg_size, timeout );
}
