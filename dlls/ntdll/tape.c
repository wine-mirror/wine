/*
 * TAPE support
 *
 * Copyright 2006 Hans Leidekker
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

#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MTIO_H
#include <sys/mtio.h>
#endif

/* FreeBSD, for example, has MTCOMP instead of MTCOMPRESSION. */
#if !defined(MTCOMPRESSION) && defined(MTCOMP)
#define MTCOMPRESSION MTCOMP
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddtape.h"
#include "ntdll_misc.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tape);

static const char *io2str( DWORD io )
{
    switch (io)
    {
#define X(x)    case (x): return #x;
    X(IOCTL_TAPE_CHECK_VERIFY);
    X(IOCTL_TAPE_CREATE_PARTITION);
    X(IOCTL_TAPE_ERASE);
    X(IOCTL_TAPE_FIND_NEW_DEVICES);
    X(IOCTL_TAPE_GET_DRIVE_PARAMS);
    X(IOCTL_TAPE_GET_MEDIA_PARAMS);
    X(IOCTL_TAPE_GET_POSITION);
    X(IOCTL_TAPE_GET_STATUS);
    X(IOCTL_TAPE_PREPARE);
    X(IOCTL_TAPE_SET_DRIVE_PARAMS);
    X(IOCTL_TAPE_SET_MEDIA_PARAMS);
    X(IOCTL_TAPE_SET_POSITION);
    X(IOCTL_TAPE_WRITE_MARKS);
#undef X
    default: { static char tmp[32]; sprintf(tmp, "IOCTL_TAPE_%ld\n", io); return tmp; }
    }
}

/******************************************************************
 *      TAPE_GetStatus
 */
static NTSTATUS TAPE_GetStatus( int error )
{
    if (!error) return STATUS_SUCCESS;
    return FILE_GetNtStatus();
}

/******************************************************************
 *      TAPE_CreatePartition
 */
static NTSTATUS TAPE_CreatePartition( int fd, TAPE_CREATE_PARTITION *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtop cmd;

    TRACE( "fd: %d method: 0x%08lx count: 0x%08lx size: 0x%08lx\n",
           fd, data->Method, data->Count, data->Size );

    if (data->Count > 1)
    {
        WARN( "Creating more than 1 partition is not supported\n" );
        return STATUS_INVALID_PARAMETER;
    }

    switch (data->Method)
    {
#ifdef MTMKPART
    case TAPE_FIXED_PARTITIONS:
    case TAPE_SELECT_PARTITIONS:
        cmd.mt_op = MTMKPART;
        cmd.mt_count = 0;
        break;
    case TAPE_INITIATOR_PARTITIONS:
        cmd.mt_op = MTMKPART;
        cmd.mt_count = data->Size;
        break;
#endif
    default:
        ERR( "Unhandled method: 0x%08lx\n", data->Method );
        return STATUS_INVALID_PARAMETER;
    }

    return TAPE_GetStatus( ioctl( fd, MTIOCTOP, &cmd ) );
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *      TAPE_Erase
 */
static NTSTATUS TAPE_Erase( int fd, TAPE_ERASE *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtop cmd;

    TRACE( "fd: %d type: 0x%08lx immediate: 0x%02x\n",
           fd, data->Type, data->Immediate );

    switch (data->Type)
    {
    case TAPE_ERASE_LONG:
        cmd.mt_op = MTERASE;
        cmd.mt_count = 1;
        break;
    case TAPE_ERASE_SHORT:
        cmd.mt_op = MTERASE;
        cmd.mt_count = 0;
        break;
    default:
        ERR( "Unhandled type: 0x%08lx\n", data->Type );
        return STATUS_INVALID_PARAMETER;
    }

    return TAPE_GetStatus( ioctl( fd, MTIOCTOP, &cmd ) );
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *      TAPE_GetDriveParams
 */
static NTSTATUS TAPE_GetDriveParams( int fd, TAPE_GET_DRIVE_PARAMETERS *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtget get;
    NTSTATUS status;

    TRACE( "fd: %d\n", fd );

    memset( data, 0, sizeof(TAPE_GET_DRIVE_PARAMETERS) );

    status = TAPE_GetStatus( ioctl( fd, MTIOCGET, &get ) );
    if (status != STATUS_SUCCESS)
        return status;

    data->ECC = FALSE;
    data->Compression = FALSE;
    data->DataPadding = FALSE;
    data->ReportSetmarks = FALSE;
#ifdef HAVE_STRUCT_MTGET_MT_BLKSIZ
    data->DefaultBlockSize = get.mt_blksiz;
#else
    data->DefaultBlockSize = get.mt_dsreg & MT_ST_BLKSIZE_MASK;
#endif
    data->MaximumBlockSize = data->DefaultBlockSize;
    data->MinimumBlockSize = data->DefaultBlockSize;
    data->MaximumPartitionCount = 1;

    return status;
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *      TAPE_GetMediaParams
 */
static NTSTATUS TAPE_GetMediaParams( int fd, TAPE_GET_MEDIA_PARAMETERS *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtget get;
    NTSTATUS status;

    TRACE( "fd: %d\n", fd );

    memset( data, 0, sizeof(TAPE_GET_MEDIA_PARAMETERS) );

    status = TAPE_GetStatus( ioctl( fd, MTIOCGET, &get ) );
    if (status != STATUS_SUCCESS)
        return status;

    data->Capacity.u.LowPart = 1024 * 1024 * 1024;
    data->Remaining.u.LowPart = 1024 * 1024 * 1024;
#ifdef HAVE_STRUCT_MTGET_MT_BLKSIZ
    data->BlockSize = get.mt_blksiz;
#else
    data->BlockSize = get.mt_dsreg & MT_ST_BLKSIZE_MASK;
#endif
    data->PartitionCount = 1;
#ifdef HAVE_STRUCT_MTGET_MT_GSTAT
    data->WriteProtected = GMT_WR_PROT(get.mt_gstat);
#else
    data->WriteProtected = 0;
#endif

    return status;
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *      TAPE_GetPosition
 */
static NTSTATUS TAPE_GetPosition( int fd, ULONG type, TAPE_GET_POSITION *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtget get;
#ifndef HAVE_STRUCT_MTGET_MT_BLKNO
    struct mtpos pos;
#endif
    NTSTATUS status;

    TRACE( "fd: %d type: 0x%08lx\n", fd, type );

    memset( data, 0, sizeof(TAPE_GET_POSITION) );

    status = TAPE_GetStatus( ioctl( fd, MTIOCGET, &get ) );
    if (status != STATUS_SUCCESS)
        return status;

#ifndef HAVE_STRUCT_MTGET_MT_BLKNO
    status = TAPE_GetStatus( ioctl( fd, MTIOCPOS, &pos ) );
    if (status != STATUS_SUCCESS)
        return status;
#endif

    switch (type)
    {
    case TAPE_ABSOLUTE_BLOCK:
        data->Type = type;
        data->Partition = get.mt_resid;
#ifdef HAVE_STRUCT_MTGET_MT_BLKNO
        data->OffsetLow = get.mt_blkno;
#else
        data->OffsetLow = pos.mt_blkno;
#endif
        break;
    case TAPE_LOGICAL_BLOCK:
    case TAPE_PSEUDO_LOGICAL_BLOCK:
        WARN( "Positioning type not supported\n" );
        break;
    default:
        ERR( "Unhandled type: 0x%08lx\n", type );
        return STATUS_INVALID_PARAMETER;
    }

    return status;
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *      TAPE_Prepare
 */
static NTSTATUS TAPE_Prepare( int fd, TAPE_PREPARE *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtop cmd;

    TRACE( "fd: %d type: 0x%08lx immediate: 0x%02x\n",
           fd, data->Operation, data->Immediate );

    switch (data->Operation)
    {
#ifdef MTLOAD
    case TAPE_LOAD:
        cmd.mt_op = MTLOAD;
        break;
#endif
#ifdef MTUNLOAD
    case TAPE_UNLOAD:
        cmd.mt_op = MTUNLOAD;
        break;
#endif
#ifdef MTRETEN
    case TAPE_TENSION:
        cmd.mt_op = MTRETEN;
        break;
#endif
#ifdef MTLOCK
    case TAPE_LOCK:
        cmd.mt_op = MTLOCK;
        break;
#endif
#ifdef MTUNLOCK
    case TAPE_UNLOCK:
        cmd.mt_op = MTUNLOCK;
        break;
#endif
    case TAPE_FORMAT:
        /* Native ignores this if the drive doesn't support it */
        return STATUS_SUCCESS;
    default:
        ERR( "Unhandled operation: 0x%08lx\n", data->Operation );
        return STATUS_INVALID_PARAMETER;
    }

    return TAPE_GetStatus( ioctl( fd, MTIOCTOP, &cmd ) );
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *      TAPE_SetDriveParams
 */
static NTSTATUS TAPE_SetDriveParams( int fd, TAPE_SET_DRIVE_PARAMETERS *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtop cmd;

    TRACE( "fd: %d ECC: 0x%02x, compression: 0x%02x padding: 0x%02x\n",
            fd, data->ECC, data->Compression, data->DataPadding );
    TRACE( "setmarks: 0x%02x zonesize: 0x%08lx\n",
           data->ReportSetmarks, data->EOTWarningZoneSize );

    if (data->ECC || data->DataPadding || data->ReportSetmarks ||
        data->EOTWarningZoneSize ) WARN( "Setting not supported\n" );

    cmd.mt_op = MTCOMPRESSION;
    cmd.mt_count = data->Compression;

    return TAPE_GetStatus( ioctl( fd, MTIOCTOP, &cmd ) );
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}
        
/******************************************************************
 *      TAPE_SetMediaParams
 */
static NTSTATUS TAPE_SetMediaParams( int fd, TAPE_SET_MEDIA_PARAMETERS *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtop cmd;

    TRACE( "fd: %d blocksize: 0x%08lx\n", fd, data->BlockSize );
    
    cmd.mt_op = MTSETBLK;
    cmd.mt_count = data->BlockSize;

    return TAPE_GetStatus( ioctl( fd, MTIOCTOP, &cmd ) );
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}
        
/******************************************************************
 *      TAPE_SetPosition
 */
static NTSTATUS TAPE_SetPosition( int fd, TAPE_SET_POSITION *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtop cmd;

    TRACE( "fd: %d method: 0x%08lx partition: 0x%08lx offset: 0x%08lx immediate: 0x%02x\n",
           fd, data->Method, data->Partition, data->Offset.u.LowPart, data->Immediate );

    if (data->Offset.u.HighPart > 0)
        return STATUS_INVALID_PARAMETER;

    switch (data->Method)
    {
    case TAPE_REWIND:
        cmd.mt_op = MTREW;
        break;
#ifdef MTSEEK
    case TAPE_ABSOLUTE_BLOCK:
        cmd.mt_op = MTSEEK;
        cmd.mt_count = data->Offset.u.LowPart;
        break;
#endif
#ifdef MTEOM
    case TAPE_SPACE_END_OF_DATA:
        cmd.mt_op = MTEOM;
        break;
#endif
    case TAPE_SPACE_FILEMARKS:
        if (data->Offset.u.LowPart >= 0) {
            cmd.mt_op = MTFSF;
            cmd.mt_count = data->Offset.u.LowPart;
        }
        else {
            cmd.mt_op = MTBSF;
            cmd.mt_count = -data->Offset.u.LowPart;
        }
        break;
    case TAPE_SPACE_SETMARKS:
        if (data->Offset.u.LowPart >= 0) {
            cmd.mt_op = MTFSS;
            cmd.mt_count = data->Offset.u.LowPart;
        }
        else {
            cmd.mt_op = MTBSS;
            cmd.mt_count = -data->Offset.u.LowPart;
        }
        break;
    case TAPE_LOGICAL_BLOCK:
    case TAPE_PSEUDO_LOGICAL_BLOCK:
    case TAPE_SPACE_RELATIVE_BLOCKS:
    case TAPE_SPACE_SEQUENTIAL_FMKS:
    case TAPE_SPACE_SEQUENTIAL_SMKS:
        WARN( "Positioning method not supported\n" );
        return STATUS_INVALID_PARAMETER;
    default:
        ERR( "Unhandled method: 0x%08lx\n", data->Method );
        return STATUS_INVALID_PARAMETER;
    }

    return TAPE_GetStatus( ioctl( fd, MTIOCTOP, &cmd ) );
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *      TAPE_WriteMarks
 */
static NTSTATUS TAPE_WriteMarks( int fd, TAPE_WRITE_MARKS *data )
{
#ifdef HAVE_SYS_MTIO_H
    struct mtop cmd;

    TRACE( "fd: %d type: 0x%08lx count: 0x%08lx immediate: 0x%02x\n",
           fd, data->Type, data->Count, data->Immediate );

    switch (data->Type)
    {
#ifdef MTWSM
    case TAPE_SETMARKS:
        cmd.mt_op = MTWSM;
        cmd.mt_count = data->Count;
        break;
#endif
    case TAPE_FILEMARKS:
    case TAPE_SHORT_FILEMARKS:
    case TAPE_LONG_FILEMARKS:
        cmd.mt_op = MTWEOF;
        cmd.mt_count = data->Count;
        break;
    default:
        ERR( "Unhandled type: 0x%08lx\n", data->Type );
        return STATUS_INVALID_PARAMETER;
    }

    return TAPE_GetStatus( ioctl( fd, MTIOCTOP, &cmd ) );
#else
    FIXME( "Not implemented.\n" );
    return STATUS_NOT_SUPPORTED;
#endif
}

/******************************************************************
 *		TAPE_DeviceIoControl
 *
 * SEE ALSO
 *   NtDeviceIoControl.
 */
NTSTATUS TAPE_DeviceIoControl( HANDLE device, HANDLE event,
    PIO_APC_ROUTINE apc, PVOID apc_user, PIO_STATUS_BLOCK io_status,
    ULONG io_control, LPVOID in_buffer, DWORD in_size,
    LPVOID out_buffer, DWORD out_size )
{
    DWORD sz = 0;
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    int fd;

    TRACE( "%p %s %p %ld %p %ld %p\n", device, io2str(io_control),
           in_buffer, in_size, out_buffer, out_size, io_status );

    io_status->Information = 0;

    if ((status = wine_server_handle_to_fd( device, 0, &fd, NULL )))
        goto error;

    switch (io_control)
    {
    case IOCTL_TAPE_CREATE_PARTITION:
        status = TAPE_CreatePartition( fd, (TAPE_CREATE_PARTITION *)in_buffer );
        break;
    case IOCTL_TAPE_ERASE:
        status = TAPE_Erase( fd, (TAPE_ERASE *)in_buffer );
        break;
    case IOCTL_TAPE_GET_DRIVE_PARAMS:
        status = TAPE_GetDriveParams( fd, (TAPE_GET_DRIVE_PARAMETERS *)out_buffer );
        break;
    case IOCTL_TAPE_GET_MEDIA_PARAMS:
        status = TAPE_GetMediaParams( fd, (TAPE_GET_MEDIA_PARAMETERS *)out_buffer );
        break;
    case IOCTL_TAPE_GET_POSITION:
        status = TAPE_GetPosition( fd, ((TAPE_GET_POSITION *)in_buffer)->Type,
                                   (TAPE_GET_POSITION *)out_buffer );
        break;
    case IOCTL_TAPE_GET_STATUS:
        status = FILE_GetNtStatus();
        break;
    case IOCTL_TAPE_PREPARE:
        status = TAPE_Prepare( fd, (TAPE_PREPARE *)in_buffer );
        break;
    case IOCTL_TAPE_SET_DRIVE_PARAMS:
        status = TAPE_SetDriveParams( fd, (TAPE_SET_DRIVE_PARAMETERS *)in_buffer );
        break;
    case IOCTL_TAPE_SET_MEDIA_PARAMS:
        status = TAPE_SetMediaParams( fd, (TAPE_SET_MEDIA_PARAMETERS *)in_buffer );
        break;
    case IOCTL_TAPE_SET_POSITION:
        status = TAPE_SetPosition( fd, (TAPE_SET_POSITION *)in_buffer );
        break;
    case IOCTL_TAPE_WRITE_MARKS:
        status = TAPE_WriteMarks( fd, (TAPE_WRITE_MARKS *)in_buffer );
        break;

    case IOCTL_TAPE_CHECK_VERIFY:
    case IOCTL_TAPE_FIND_NEW_DEVICES:
        break;
    default:
        FIXME( "Unsupported IOCTL %lx (type=%lx access=%lx func=%lx meth=%lx)\n",
               io_control, io_control >> 16, (io_control >> 14) & 3,
               (io_control >> 2) & 0xfff, io_control & 3 );
        break;
    }

    wine_server_release_fd( device, fd );

error:
    io_status->u.Status = status;
    io_status->Information = sz;
    if (event) NtSetEvent( event, NULL );
    return status;
}
