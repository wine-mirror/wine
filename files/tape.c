/*
 * Tape handling functions
 *
 * Copyright 1999 Chris Morgan <cmorgan@wpi.edu>
 *                James Abbatiello <abbeyj@wpi.edu>
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
 *
 * TODO:
 *    Everything, all functions are stubs.
 */

#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tape);


/************************************************************************
 *		BackupRead  (KERNEL32.@)
 */
BOOL WINAPI BackupRead( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead, BOOL bAbort,
		BOOL bProcessSecurity, LPVOID *lpContext )
{
  FIXME("(%p, %p, %ld, %p, %d, %d, %p) stub!\n", hFile, lpBuffer,
    nNumberOfBytesToRead, lpNumberOfBytesRead, bAbort, bProcessSecurity,
    lpContext);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		BackupSeek  (KERNEL32.@)
 */
BOOL WINAPI BackupSeek( HANDLE hFile, DWORD dwLowBytesToSeek, DWORD dwHighBytesToSeek,
         	LPDWORD lpdwLowByteSeeked, LPDWORD lpdwHighByteSeeked,
		LPVOID *lpContext )
{
  FIXME("(%p, %ld, %ld, %p, %p, %p) stub!\n", hFile, dwLowBytesToSeek,
    dwHighBytesToSeek, lpdwLowByteSeeked, lpdwHighByteSeeked, lpContext);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		BackupWrite  (KERNEL32.@)
 */
BOOL WINAPI BackupWrite( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten, BOOL bAbort,
		BOOL bProcessSecurity, LPVOID *lpContext )
{
  FIXME("(%p, %p, %ld, %p, %d, %d, %p) stub!\n", hFile, lpBuffer,
    nNumberOfBytesToWrite, lpNumberOfBytesWritten, bAbort,
    bProcessSecurity, lpContext);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		CreateTapePartition  (KERNEL32.@)
 */
DWORD WINAPI CreateTapePartition( HANDLE hDevice, DWORD dwPartitionMethod,
			DWORD dwCount, DWORD dwSize )
{
  FIXME("(%p, %ld, %ld, %ld) stub!\n", hDevice, dwPartitionMethod, dwCount,
    dwSize);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		EraseTape  (KERNEL32.@)
 */
DWORD WINAPI EraseTape( HANDLE hDevice, DWORD dwEraseType, BOOL bImmediate )
{
  FIXME("(%p, %ld, %d) stub!\n", hDevice, dwEraseType, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		GetTapeParameters  (KERNEL32.@)
 */
DWORD WINAPI GetTapeParameters( HANDLE hDevice, DWORD dwOperation,
			LPDWORD lpdwSize, LPVOID lpTapeInformation )
{
  FIXME("(%p, %ld, %p, %p) stub!\n", hDevice, dwOperation, lpdwSize,
    lpTapeInformation);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		GetTapePosition  (KERNEL32.@)
 */
DWORD WINAPI GetTapePosition( HANDLE hDevice, DWORD dwPositionType,
			LPDWORD lpdwPartition, LPDWORD lpdwOffsetLow,
			LPDWORD lpdwOffsetHigh )
{
  FIXME("(%p, %ld, %p, %p, %p) stub!\n", hDevice, dwPositionType,
    lpdwPartition, lpdwOffsetLow, lpdwOffsetHigh);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		GetTapeStatus  (KERNEL32.@)
 */
DWORD WINAPI GetTapeStatus( HANDLE hDevice )
{
  FIXME("(%p) stub!\n", hDevice);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		PrepareTape  (KERNEL32.@)
 */
DWORD WINAPI PrepareTape( HANDLE hDevice,  DWORD dwOperation,  BOOL bImmediate )
{
  FIXME("(%p, %ld, %d) stub!\n", hDevice, dwOperation, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		SetTapeParameters  (KERNEL32.@)
 */
DWORD WINAPI SetTapeParameters( HANDLE hDevice, DWORD dwOperation,
                         LPVOID lpTapeInformation )
{
  FIXME("(%p, %ld, %p) stub!\n", hDevice, dwOperation, lpTapeInformation);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		SetTapePosition  (KERNEL32.@)
 */
DWORD WINAPI SetTapePosition( HANDLE hDevice, DWORD dwPositionMethod, DWORD
                       dwPartition, DWORD dwOffsetLow, DWORD dwOffsetHigh,
                       BOOL bImmediate )
{
  FIXME("(%p, %ld, %ld, %ld, %ld, %d) stub!\n", hDevice, dwPositionMethod,
    dwPartition, dwOffsetLow, dwOffsetHigh, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		WriteTapemark  (KERNEL32.@)
 */
DWORD WINAPI WriteTapemark( HANDLE hDevice,  DWORD dwTapemarkType, DWORD
                     dwTapemarkCount, BOOL bImmediate )
{
  FIXME("(%p, %ld, %ld, %d) stub!\n", hDevice, dwTapemarkType,
    dwTapemarkCount, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}
