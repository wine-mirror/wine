/*
 * Tape handling functions
 *
 * Copyright 1999 Chris Morgan <cmorgan@wpi.edu>
 *                James Abbatiello <abbeyj@wpi.edu>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "winerror.h"
#include "winioctl.h"
#include "ddk/ntddtape.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tape);

static DWORD set_error_from_status( NTSTATUS status )
{
    DWORD error = RtlNtStatusToDosError( status );

    SetLastError( error );
    return error;
}

/************************************************************************
 *		BackupRead  (KERNEL32.@)
 *
 * Backup a file or directory.
 */
BOOL WINAPI BackupRead( HANDLE file, LPBYTE buffer, DWORD to_read,
    LPDWORD read, BOOL abort, BOOL security, LPVOID *context )
{
    FIXME( "(%p, %p, %ld, %p, %d, %d, %p)\n", file, buffer,
           to_read, read, abort, security, context );
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


/************************************************************************
 *		BackupSeek  (KERNEL32.@)
 *
 * Seek forward in a backup stream.
 */
BOOL WINAPI BackupSeek( HANDLE file, DWORD seek_low, DWORD seek_high,
    LPDWORD seeked_low, LPDWORD seeked_high, LPVOID *context )
{
    FIXME( "(%p, %ld, %ld, %p, %p, %p)\n", file, seek_low,
           seek_high, seeked_low, seeked_high, context );
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


/************************************************************************
 *		BackupWrite  (KERNEL32.@)
 *
 * Restore a file or directory.
 */
BOOL WINAPI BackupWrite( HANDLE file, LPBYTE buffer, DWORD to_write,
    LPDWORD written, BOOL abort, BOOL security, LPVOID *context )
{
    FIXME( "(%p, %p, %ld, %p, %d, %d, %p)\n", file, buffer,
           to_write, written, abort, security, context );
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


/************************************************************************
 *		CreateTapePartition  (KERNEL32.@)
 *
 * Format a tape.
 */
DWORD WINAPI CreateTapePartition( HANDLE device, DWORD method,
    DWORD count, DWORD size )
{
    NTSTATUS status;
    TAPE_CREATE_PARTITION part;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %ld, %ld)\n", device, method, count, size );

    part.Method = method;
    part.Count = count;
    part.Size = size;

    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
        IOCTL_TAPE_CREATE_PARTITION, &part, sizeof(TAPE_CREATE_PARTITION), NULL, 0 );

    return set_error_from_status( status );
}


/************************************************************************
 *		EraseTape  (KERNEL32.@)
 *
 * Erase a tape.
 */
DWORD WINAPI EraseTape( HANDLE device, DWORD type, BOOL immediate )
{
    NTSTATUS status;
    TAPE_ERASE erase;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %d)\n", device, type, immediate );

    erase.Type = type;
    erase.Immediate = immediate;
            
    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
        IOCTL_TAPE_ERASE, &erase, sizeof(TAPE_ERASE), NULL, 0 );

    return set_error_from_status( status );
}


/************************************************************************
 *		GetTapeParameters  (KERNEL32.@)
 *
 * Retrieve information about a tape or tape drive.
 */
DWORD WINAPI GetTapeParameters( HANDLE device, DWORD operation,
    LPDWORD size, LPVOID info )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %p, %p)\n", device, operation, size, info );

    switch (operation)
    {
    case GET_TAPE_DRIVE_INFORMATION:
        status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
            IOCTL_TAPE_GET_DRIVE_PARAMS, NULL, 0, info, *size );
        break;
    case GET_TAPE_MEDIA_INFORMATION:
        status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
            IOCTL_TAPE_GET_MEDIA_PARAMS, NULL, 0, info, *size ); 
        break;
    default:
        ERR( "Unhandled operation: 0x%08lx\n", operation );
    }

    return set_error_from_status( status );
}


/************************************************************************
 *		GetTapePosition  (KERNEL32.@)
 *
 *	Retrieve the current tape position.
 */
DWORD WINAPI GetTapePosition( HANDLE device, DWORD type, LPDWORD partition,
    LPDWORD offset_low, LPDWORD offset_high )
{
    NTSTATUS status;
    TAPE_GET_POSITION in, out;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %p, %p, %p)\n", device, type, partition, offset_low,
           offset_high );

    memset( &in, 0, sizeof(TAPE_GET_POSITION) );
    in.Type = type;

    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
        IOCTL_TAPE_GET_POSITION, &in, sizeof(TAPE_GET_POSITION),
        &out, sizeof(TAPE_GET_POSITION) );

    if (status != STATUS_SUCCESS)
        return set_error_from_status( status );

    *partition = out.Partition;
    *offset_low = out.OffsetLow;
    *offset_high = out.OffsetHigh;

    return set_error_from_status( status );
}


/************************************************************************
 *		GetTapeStatus  (KERNEL32.@)
 *
 * Determine if the tape device is ready for operation.
 */
DWORD WINAPI GetTapeStatus( HANDLE device )
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    TRACE( "(%p)\n", device );

    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
        IOCTL_TAPE_GET_STATUS, NULL, 0, NULL, 0 );

    return set_error_from_status( status );
}


/************************************************************************
 *		PrepareTape  (KERNEL32.@)
 *
 * Prepare a tape for operation.
 */
DWORD WINAPI PrepareTape( HANDLE device, DWORD operation, BOOL immediate )
{
    NTSTATUS status;
    TAPE_PREPARE prep;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %d)\n", device, operation, immediate );

    prep.Operation = operation;
    prep.Immediate = immediate;

    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
        IOCTL_TAPE_PREPARE, &prep, sizeof(TAPE_PREPARE), NULL, 0 );

    return set_error_from_status( status );
}


/************************************************************************
 *		SetTapeParameters  (KERNEL32.@)
 *
 * Configure a tape or tape device.
 */
DWORD WINAPI SetTapeParameters( HANDLE device, DWORD operation, LPVOID info )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %p)\n", device, operation, info );

    switch (operation)
    {
    case SET_TAPE_DRIVE_INFORMATION:
        status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
            IOCTL_TAPE_SET_DRIVE_PARAMS, info, sizeof(TAPE_SET_DRIVE_PARAMETERS),
            NULL, 0 );
        break;
    case SET_TAPE_MEDIA_INFORMATION:
        status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
            IOCTL_TAPE_SET_MEDIA_PARAMS, info, sizeof(TAPE_SET_MEDIA_PARAMETERS),
            NULL, 0 );
        break;
    default:
        ERR( "Unhandled operation: 0x%08lx\n", operation );
    }

    return set_error_from_status( status );
}


/************************************************************************
 *		SetTapePosition  (KERNEL32.@)
 *
 * Set the tape position on a given device.
 */
DWORD WINAPI SetTapePosition( HANDLE device, DWORD method, DWORD partition,
    DWORD offset_low, DWORD offset_high, BOOL immediate )
{
    NTSTATUS status;
    TAPE_SET_POSITION pos;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %ld, %ld, %ld, %d)\n", device, method, partition,
           offset_low, offset_high, immediate );

    pos.Method = method;
    pos.Partition = partition;
    pos.Offset.u.LowPart = offset_low;
    pos.Offset.u.HighPart = offset_high;
    pos.Immediate = immediate;

    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
        IOCTL_TAPE_SET_POSITION, &pos, sizeof(TAPE_SET_POSITION), NULL, 0 );

    return set_error_from_status( status );
}


/************************************************************************
 *		WriteTapemark  (KERNEL32.@)
 *
 * Write tapemarks on a tape.
 */
DWORD WINAPI WriteTapemark( HANDLE device, DWORD type, DWORD count,
    BOOL immediate )
{
    NTSTATUS status;
    TAPE_WRITE_MARKS marks;
    IO_STATUS_BLOCK io;

    TRACE( "(%p, %ld, %ld, %d)\n", device, type, count, immediate );

    marks.Type = type;
    marks.Count = count;
    marks.Immediate = immediate;

    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &io,
        IOCTL_TAPE_WRITE_MARKS, &marks, sizeof(TAPE_WRITE_MARKS), NULL, 0 );

    return set_error_from_status( status );
}
