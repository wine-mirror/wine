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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"
#include "file.h" /* FIXME */

#include "winternl.h"
#include "winioctl.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


/**************************************************************************
 *                 NtOpenFile				[NTDLL.@]
 *                 ZwOpenFile				[NTDLL.@]
 * FUNCTION: Opens a file
 * ARGUMENTS:
 *  FileHandle		Variable that receives the file handle on return
 *  DesiredAccess	Access desired by the caller to the file
 *  ObjectAttributes	Structue describing the file to be opened
 *  IoStatusBlock	Receives details about the result of the operation
 *  ShareAccess		Type of shared access the caller requires
 *  OpenOptions		Options for the file open
 */
NTSTATUS WINAPI NtOpenFile(
	OUT PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions)
{
	LPWSTR filename;
	static const WCHAR szDosDevices[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};
	DOS_FULL_NAME full_name;
	NTSTATUS r;

	FIXME("(%p,0x%08lx,%p,%p,0x%08lx,0x%08lx) partial stub\n",
		FileHandle, DesiredAccess, ObjectAttributes,
		IoStatusBlock, ShareAccess, OpenOptions);

	dump_ObjectAttributes (ObjectAttributes);

	if(ObjectAttributes->RootDirectory)
	{
		FIXME("Object root directory unknown %p\n",
			ObjectAttributes->RootDirectory);
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	filename = ObjectAttributes->ObjectName->Buffer;

	/* FIXME: DOSFS stuff should call here, not vice-versa */
	if(strncmpW(filename, szDosDevices, strlenW(szDosDevices)))
		return STATUS_OBJECT_NAME_NOT_FOUND;

	/* FIXME: this calls SetLastError() -> bad */
	if(!DOSFS_GetFullName(&filename[strlenW(szDosDevices)], TRUE,
				&full_name))
		return STATUS_OBJECT_NAME_NOT_FOUND;

	/* FIXME: modify server protocol so
                  create file takes an OBJECT_ATTRIBUTES structure */
        SERVER_START_REQ( create_file )
        {
            req->access     = DesiredAccess;
            req->inherit    = 0;
            req->sharing    = ShareAccess;
            req->create     = OPEN_EXISTING;
            req->attrs      = 0;
            req->drive_type = GetDriveTypeW( full_name.short_name );
            wine_server_add_data( req, full_name.long_name, strlen(full_name.long_name) );
            SetLastError(0);
            r = wine_server_call( req );
            *FileHandle = reply->handle;
        }
        SERVER_END_REQ;

	return r;
}

/**************************************************************************
 *		NtCreateFile				[NTDLL.@]
 *		ZwCreateFile				[NTDLL.@]
 * FUNCTION: Either causes a new file or directory to be created, or it opens
 *  an existing file, device, directory or volume, giving the caller a handle
 *  for the file object. This handle can be used by subsequent calls to
 *  manipulate data within the file or the file object's state of attributes.
 * ARGUMENTS:
 *	FileHandle		Points to a variable which receives the file handle on return
 *	DesiredAccess		Desired access to the file
 *	ObjectAttributes	Structure describing the file
 *	IoStatusBlock		Receives information about the operation on return
 *	AllocationSize		Initial size of the file in bytes
 *	FileAttributes		Attributes to create the file with
 *	ShareAccess		Type of shared access the caller would like to the file
 *	CreateDisposition	Specifies what to do, depending on whether the file already exists
 *	CreateOptions		Options for creating a new file
 *	EaBuffer		Undocumented
 *	EaLength		Undocumented
 */
NTSTATUS WINAPI NtCreateFile(
	OUT PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocateSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength)
{
	FIXME("(%p,0x%08lx,%p,%p,%p,0x%08lx,0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx) stub\n",
	FileHandle,DesiredAccess,ObjectAttributes,
	IoStatusBlock,AllocateSize,FileAttributes,
	ShareAccess,CreateDisposition,CreateOptions,EaBuffer,EaLength);
	dump_ObjectAttributes (ObjectAttributes);
	return 0;
}

/* set the last error depending on errno */
NTSTATUS NTFILE_errno_to_status(int val)
{
    switch (val)
    {
    case EAGAIN:    return ( STATUS_SHARING_VIOLATION );
    case ESPIPE:
    case EBADF:     return ( STATUS_INVALID_HANDLE );
    case ENOSPC:    return ( STATUS_DISK_FULL );
    case EACCES:
    case ESRCH:
    case EPERM:     return ( STATUS_ACCESS_DENIED );
    case EROFS:     return ( STATUS_MEDIA_WRITE_PROTECTED );
    case EBUSY:     return ( STATUS_FILE_LOCK_CONFLICT );
    case ENOENT:    return ( STATUS_NO_SUCH_FILE );
    case EISDIR:    return ( STATUS_FILE_IS_A_DIRECTORY );
    case ENFILE:
    case EMFILE:    return ( STATUS_NO_MORE_FILES );
    case EEXIST:    return ( STATUS_OBJECT_NAME_COLLISION );
    case EINVAL:    return ( STATUS_INVALID_PARAMETER );
    case ENOTEMPTY: return ( STATUS_DIRECTORY_NOT_EMPTY );
    case EIO:       return ( STATUS_ACCESS_VIOLATION );
    }
    perror("file_set_error");
    return ( STATUS_INVALID_PARAMETER );
}


/******************************************************************************
 *  NtReadFile					[NTDLL.@]
 *  ZwReadFile					[NTDLL.@]
 *
 * Parameters
 *   HANDLE32 		FileHandle
 *   HANDLE32 		Event 		OPTIONAL
 *   PIO_APC_ROUTINE 	ApcRoutine 	OPTIONAL
 *   PVOID 		ApcContext 	OPTIONAL
 *   PIO_STATUS_BLOCK 	IoStatusBlock
 *   PVOID 		Buffer
 *   ULONG 		Length
 *   PLARGE_INTEGER 	ByteOffset 	OPTIONAL
 *   PULONG 		Key 		OPTIONAL
 *
 * IoStatusBlock->Information contains the number of bytes read on return.
 */
NTSTATUS WINAPI NtReadFile (
	HANDLE FileHandle,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset,
	PULONG Key)
{
	int fd, result, flags, ret;
	enum fd_type type;

	FIXME("(%p,%p,%p,%p,%p,%p,0x%08lx,%p,%p),partial stub!\n",
		FileHandle,EventHandle,ApcRoutine,ApcContext,IoStatusBlock,Buffer,Length,ByteOffset,Key);

	if (IsBadWritePtr( Buffer, Length ) ||
	    IsBadWritePtr( IoStatusBlock, sizeof(*IoStatusBlock)) ||
	    IsBadWritePtr( ByteOffset, sizeof(*ByteOffset)) )
		return STATUS_ACCESS_VIOLATION;

	IoStatusBlock->Information = 0;

	ret = wine_server_handle_to_fd( FileHandle, GENERIC_READ, &fd, &type, &flags );
	if(ret)
		return ret;

	/* FIXME: this code only does synchronous reads so far */

	/* FIXME: depending on how libc implements this, between two processes
	      there could be a race condition between the seek and read here */
	do
	{
               	result = pread( fd, Buffer, Length, ByteOffset->QuadPart);
	}
	while ( (result == -1) && ((errno == EAGAIN) || (errno == EINTR)) );

	close( fd );

	if (result == -1)
	{
		return IoStatusBlock->u.Status = NTFILE_errno_to_status(errno);
	}

	IoStatusBlock->Information = result;
	IoStatusBlock->u.Status = 0;

	return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtWriteFile					[NTDLL.@]
 *  ZwWriteFile					[NTDLL.@]
 *
 * Parameters
 *   HANDLE32 		FileHandle
 *   HANDLE32 		Event 		OPTIONAL
 *   PIO_APC_ROUTINE 	ApcRoutine 	OPTIONAL
 *   PVOID 		ApcContext 	OPTIONAL
 *   PIO_STATUS_BLOCK 	IoStatusBlock
 *   PVOID 		Buffer
 *   ULONG 		Length
 *   PLARGE_INTEGER 	ByteOffset 	OPTIONAL
 *   PULONG 		Key 		OPTIONAL
 */
NTSTATUS WINAPI NtWriteFile (
	HANDLE FileHandle,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset,
	PULONG Key)
{
	FIXME("(%p,%p,%p,%p,%p,%p,0x%08lx,%p,%p),stub!\n",
	FileHandle,EventHandle,ApcRoutine,ApcContext,IoStatusBlock,Buffer,Length,ByteOffset,Key);
	return 0;
}

/**************************************************************************
 *		NtDeviceIoControlFile			[NTDLL.@]
 *		ZwDeviceIoControlFile			[NTDLL.@]
 */
NTSTATUS WINAPI NtDeviceIoControlFile(HANDLE DeviceHandle, HANDLE hEvent,
                                      PIO_APC_ROUTINE UserApcRoutine, 
                                      PVOID UserApcContext,
                                      PIO_STATUS_BLOCK IoStatusBlock,
                                      ULONG IoControlCode,
                                      PVOID InputBuffer,
                                      ULONG InputBufferSize,
                                      PVOID OutputBuffer,
                                      ULONG OutputBufferSize)
{
    DWORD               clientID = 0;
    char                str[3];

    TRACE("(%p,%p,%p,%p,%p,0x%08lx,%p,0x%08lx,%p,0x%08lx)\n",
          DeviceHandle, hEvent, UserApcRoutine, UserApcContext,
          IoStatusBlock, IoControlCode, 
          InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize);

    /* FIXME: clientID hack should disappear */
    SERVER_START_REQ( get_device_id )
    {
        req->handle = DeviceHandle;
        if (!wine_server_call( req )) clientID = reply->id;
    }
    SERVER_END_REQ;

    if (!clientID) return STATUS_INVALID_PARAMETER;
    strcpy(str,  "A:");
    str[0] += LOBYTE(clientID);
    
    /* FIXME: should use the NTDLL equivalent */
    if (GetDriveTypeA(str) == DRIVE_CDROM)
    {
        return CDROM_DeviceIoControl(clientID, DeviceHandle, hEvent, 
                                     UserApcRoutine, UserApcContext, 
                                     IoStatusBlock, IoControlCode,
                                     InputBuffer, InputBufferSize,
                                     OutputBuffer, OutputBufferSize);
    }
    
    FIXME("Unimplemented dwIoControlCode=%08lx\n", IoControlCode);
    IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;
    IoStatusBlock->Information = 0;
    if (hEvent) NtSetEvent(hEvent, NULL);
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 * NtFsControlFile [NTDLL.@]
 * ZwFsControlFile [NTDLL.@]
 */
NTSTATUS WINAPI NtFsControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG IoControlCode,
	IN PVOID InputBuffer,
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize)
{
	FIXME("(%p,%p,%p,%p,%p,0x%08lx,%p,0x%08lx,%p,0x%08lx): stub\n",
	DeviceHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,IoControlCode,
	InputBuffer,InputBufferSize,OutputBuffer,OutputBufferSize);
	return 0;
}

/******************************************************************************
 *  NtSetVolumeInformationFile		[NTDLL.@]
 *  ZwSetVolumeInformationFile		[NTDLL.@]
 */
NTSTATUS WINAPI NtSetVolumeInformationFile(
	IN HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FsInformation,
        ULONG Length,
	FS_INFORMATION_CLASS FsInformationClass)
{
	FIXME("(%p,%p,%p,0x%08lx,0x%08x) stub\n",
	FileHandle,IoStatusBlock,FsInformation,Length,FsInformationClass);
	return 0;
}

/******************************************************************************
 *  NtQueryInformationFile		[NTDLL.@]
 *  ZwQueryInformationFile		[NTDLL.@]
 */
NTSTATUS WINAPI NtQueryInformationFile(HANDLE hFile, PIO_STATUS_BLOCK io_status,
                                       PVOID ptr, LONG len,
                                       FILE_INFORMATION_CLASS class)
{
    NTSTATUS    status;
    LONG        used = 0;
    BYTE        answer[256];
    time_t      ct = 0, wt = 0, at = 0;

    TRACE("(%p,%p,%p,0x%08lx,0x%08x)\n", hFile, io_status, ptr, len, class);

    switch (class)
    {
    case FileBasicInformation:
        {
            FILE_BASIC_INFORMATION*  fbi = (FILE_BASIC_INFORMATION*)answer;
            if (sizeof(answer) < sizeof(*fbi)) goto too_small;

            SERVER_START_REQ( get_file_info )
            {
                req->handle = hFile;
                if (!(status = wine_server_call( req )))
                {
                    /* FIXME: which file types are supported ?
                     * Serial ports (FILE_TYPE_CHAR) are not,
                     * and MSDN also says that pipes are not supported.
                     * FILE_TYPE_REMOTE seems to be supported according to
                     * MSDN q234741.txt */
                    if ((reply->type == FILE_TYPE_DISK) ||
                        (reply->type == FILE_TYPE_REMOTE))
                    {
                        at = reply->access_time;
                        wt = reply->write_time;
                        ct = reply->change_time;
                        fbi->FileAttributes = reply->attr;
                        used = sizeof(*fbi);
                    }
                    else status = STATUS_INVALID_HANDLE; /* FIXME ??? */
                }
            }
            SERVER_END_REQ;
            if (used)
            {
                RtlSecondsSince1970ToTime(wt, &fbi->CreationTime);
                RtlSecondsSince1970ToTime(wt, &fbi->LastWriteTime);
                RtlSecondsSince1970ToTime(ct, &fbi->ChangeTime);
                RtlSecondsSince1970ToTime(at, &fbi->LastAccessTime);
            }
        }
        break;
    case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION*  fsi = (FILE_STANDARD_INFORMATION*)answer;
            if (sizeof(answer) < sizeof(*fsi)) goto too_small;

            SERVER_START_REQ( get_file_info )
            {
                req->handle = hFile;
                if (!(status = wine_server_call( req )))
                {
                    /* FIXME: which file types are supported ?
                     * Serial ports (FILE_TYPE_CHAR) are not,
                     * and MSDN also says that pipes are not supported.
                     * FILE_TYPE_REMOTE seems to be supported according to
                     * MSDN q234741.txt */
                    if ((reply->type == FILE_TYPE_DISK) ||
                        (reply->type == FILE_TYPE_REMOTE))
                    {
                        fsi->AllocationSize.s.HighPart = reply->alloc_high;
                        fsi->AllocationSize.s.LowPart  = reply->alloc_low;
                        fsi->EndOfFile.s.HighPart      = reply->size_high;
                        fsi->EndOfFile.s.LowPart       = reply->size_low;
                        fsi->NumberOfLinks             = reply->links;
                        fsi->DeletePending             = FALSE; /* FIXME */
                        fsi->Directory                 = (reply->attr & FILE_ATTRIBUTE_DIRECTORY);
                        used = sizeof(*fsi);
                    }
                    else status = STATUS_INVALID_HANDLE; /* FIXME ??? */
                }
            }
            SERVER_END_REQ;
        }
        break;
    case FilePositionInformation:
        {
            FILE_POSITION_INFORMATION*  fpi = (FILE_POSITION_INFORMATION*)answer;
            if (sizeof(answer) < sizeof(*fpi)) goto too_small;

            SERVER_START_REQ( set_file_pointer )
            {
                req->handle = hFile;
                req->low = 0;
                req->high = 0;
                req->whence = SEEK_CUR;
                if (!(status = wine_server_call( req )))
                {
                    fpi->CurrentByteOffset.s.HighPart = reply->new_high;
                    fpi->CurrentByteOffset.s.LowPart  = reply->new_low;
                    used = sizeof(*fpi);
                }
            }
            SERVER_END_REQ;
        }
        break;
    default:
        FIXME("Unsupported class (%d)\n", class);
        return io_status->u.Status = STATUS_NOT_IMPLEMENTED;
    }
    if (used) memcpy(ptr, answer, min(used, len));
    io_status->u.Status = status;
    io_status->Information = len;
    return status;
 too_small:
    io_status->Information = 0;
    return io_status->u.Status = STATUS_BUFFER_TOO_SMALL;
}

/******************************************************************************
 *  NtSetInformationFile		[NTDLL.@]
 *  ZwSetInformationFile		[NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationFile(HANDLE hFile, PIO_STATUS_BLOCK io_status,
                                     PVOID ptr, ULONG len, 
                                     FILE_INFORMATION_CLASS class)
{
    NTSTATUS    status = STATUS_INVALID_PARAMETER_3;

    TRACE("(%p,%p,%p,0x%08lx,0x%08x)\n", hFile, io_status, ptr, len, class);

    switch (class)
    {   
    case FilePositionInformation:
        if (len >= sizeof(FILE_POSITION_INFORMATION))
        {
            FILE_POSITION_INFORMATION*  fpi = (FILE_POSITION_INFORMATION*)ptr;

            SERVER_START_REQ( set_file_pointer )
            {
                req->handle = hFile;
                req->low = fpi->CurrentByteOffset.s.LowPart;
                req->high = fpi->CurrentByteOffset.s.HighPart;
                req->whence = SEEK_SET;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
            status = STATUS_SUCCESS;
        }
        break;
    default:
        FIXME("Unsupported class (%d)\n", class);
        return STATUS_NOT_IMPLEMENTED;
    }
    io_status->u.Status = status;
    io_status->Information = 0;
    return status;
}

/******************************************************************************
 *  NtQueryDirectoryFile	[NTDLL.@]
 *  ZwQueryDirectoryFile	[NTDLL.@]
 *  ZwQueryDirectoryFile
 */
NTSTATUS WINAPI NtQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan)
{
	FIXME("(%p %p %p %p %p %p 0x%08lx 0x%08x 0x%08x %p 0x%08x\n",
	FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation,
	Length, FileInformationClass, ReturnSingleEntry,
	debugstr_us(FileName),RestartScan);
	return 0;
}

/******************************************************************************
 *  NtQueryVolumeInformationFile		[NTDLL.@]
 *  ZwQueryVolumeInformationFile		[NTDLL.@]
 */
NTSTATUS WINAPI NtQueryVolumeInformationFile (
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FSInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FSInformationClass)
{
	ULONG len = 0;

	FIXME("(%p %p %p 0x%08lx 0x%08x) stub!\n",
	FileHandle, IoStatusBlock, FSInformation, Length, FSInformationClass);

	switch ( FSInformationClass )
	{
	  case FileFsVolumeInformation:
	    len = sizeof( FILE_FS_VOLUME_INFORMATION );
	    break;
	  case FileFsLabelInformation:
	    len = 0;
	    break;

	  case FileFsSizeInformation:
	    len = sizeof( FILE_FS_SIZE_INFORMATION );
	    break;

	  case FileFsDeviceInformation:
	    len = sizeof( FILE_FS_DEVICE_INFORMATION );
	    break;
	  case FileFsAttributeInformation:
	    len = sizeof( FILE_FS_ATTRIBUTE_INFORMATION );
	    break;

	  case FileFsControlInformation:
	    len = 0;
	    break;

	  case FileFsFullSizeInformation:
	    len = 0;
	    break;

	  case FileFsObjectIdInformation:
	    len = 0;
	    break;

	  case FileFsMaximumInformation:
	    len = 0;
	    break;
	}

	if (Length < len)
	  return STATUS_BUFFER_TOO_SMALL;

	switch ( FSInformationClass )
	{
	  case FileFsVolumeInformation:
	    break;
	  case FileFsLabelInformation:
	    break;

	  case FileFsSizeInformation:
	    break;

	  case FileFsDeviceInformation:
	    if (FSInformation)
	    {
	      FILE_FS_DEVICE_INFORMATION * DeviceInfo = FSInformation;
	      DeviceInfo->DeviceType = FILE_DEVICE_DISK;
	      DeviceInfo->Characteristics = 0;
	      break;
	    };
	  case FileFsAttributeInformation:
	    break;

	  case FileFsControlInformation:
	    break;

	  case FileFsFullSizeInformation:
	    break;

	  case FileFsObjectIdInformation:
	    break;

	  case FileFsMaximumInformation:
	    break;
	}
	IoStatusBlock->u.Status = STATUS_SUCCESS;
	IoStatusBlock->Information = len;
	return STATUS_SUCCESS;
}

/******************************************************************
 *		NtFlushBuffersFile  (NTDLL.@)
 */
NTSTATUS WINAPI NtFlushBuffersFile( HANDLE hFile, IO_STATUS_BLOCK* IoStatusBlock )
{
    NTSTATUS ret;
    HANDLE hEvent = NULL;

    SERVER_START_REQ( flush_file )
    {
        req->handle = hFile;
        ret = wine_server_call( req );
        hEvent = reply->event;
    }
    SERVER_END_REQ;
    if( !ret && hEvent )
    {
        ret = NtWaitForSingleObject( hEvent, FALSE, NULL );
        NtClose( hEvent );
    }
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

    if (apc || io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    for (;;)
    {
        SERVER_START_REQ( lock_file )
        {
            req->handle      = hFile;
            req->offset_low  = offset->s.LowPart;
            req->offset_high = offset->s.HighPart;
            req->count_low   = count->s.LowPart;
            req->count_high  = count->s.HighPart;
            req->shared      = !exclusive;
            req->wait        = !dont_wait;
            ret = wine_server_call( req );
            handle = reply->handle;
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
            NtWaitForMultipleObjects( 0, NULL, FALSE, FALSE, &time );
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

    TRACE( "%p %lx%08lx %lx%08lx\n",
           hFile, offset->s.HighPart, offset->s.LowPart, count->s.HighPart, count->s.LowPart );

    if (io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    SERVER_START_REQ( unlock_file )
    {
        req->handle      = hFile;
        req->offset_low  = offset->s.LowPart;
        req->offset_high = offset->s.HighPart;
        req->count_low   = count->s.LowPart;
        req->count_high  = count->s.HighPart;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}
