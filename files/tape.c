/*
 * Tape handling functions
 *
 * Copyright 1999 Chris Morgan <cmorgan@wpi.edu>
 *                James Abbatiello <abbeyj@wpi.edu>
 *
 * TODO: 
 *    Everything, all functions are stubs.
 */

#include "winbase.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(tape)


/************************************************************************
 *		BackupRead  (KERNEL32.107)
 */
BOOL WINAPI BackupRead( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead, BOOL bAbort,
		BOOL bProcessSecurity, LPVOID *lpContext )
{
  FIXME("(%04x, %p, %ld, %p, %d, %d, %p) stub!\n", hFile, lpBuffer,
    nNumberOfBytesToRead, lpNumberOfBytesRead, bAbort, bProcessSecurity,
    lpContext);
    
  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		BackupSeek  (KERNEL32.108)
 */
BOOL WINAPI BackupSeek( HANDLE hFile, DWORD dwLowBytesToSeek, DWORD dwHighBytesToSeek,
         	LPDWORD lpdwLowByteSeeked, LPDWORD lpdwHighByteSeeked,
		LPVOID *lpContext )
{
  FIXME("(%04x, %ld, %ld, %p, %p, %p) stub!\n", hFile, dwLowBytesToSeek,
    dwHighBytesToSeek, lpdwLowByteSeeked, lpdwHighByteSeeked, lpContext);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;  
}  


/************************************************************************
 *		BackupWrite  (KERNEL32.109)
 */
BOOL WINAPI BackupWrite( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten, BOOL bAbort,
		BOOL bProcessSecurity, LPVOID *lpContext )
{
  FIXME("(%04x, %p, %ld, %p, %d, %d, %p) stub!\n", hFile, lpBuffer,
    nNumberOfBytesToWrite, lpNumberOfBytesWritten, bAbort,
    bProcessSecurity, lpContext);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		CreateTapePartition  (KERNEL32.177)
 */
DWORD WINAPI CreateTapePartition( HANDLE hDevice, DWORD dwPartitionMethod,
			DWORD dwCount, DWORD dwSize )
{
  FIXME("(%04x, %ld, %ld, %ld) stub!\n", hDevice, dwPartitionMethod, dwCount,
    dwSize);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		EraseTape  (KERNEL32.212)
 */
DWORD WINAPI EraseTape( HANDLE hDevice, DWORD dwEraseType, BOOL bImmediate )
{
  FIXME("(%04x, %ld, %d) stub!\n", hDevice, dwEraseType, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		GetTapeParameters  (KERNEL32.409)
 */
DWORD WINAPI GetTapeParameters( HANDLE hDevice, DWORD dwOperation,
			LPDWORD lpdwSize, LPVOID lpTapeInformation )
{
  FIXME("(%04x, %ld, %p, %p) stub!\n", hDevice, dwOperation, lpdwSize,
    lpTapeInformation);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		GetTapePosition  (KERNEL32.410)
 */
DWORD WINAPI GetTapePosition( HANDLE hDevice, DWORD dwPositionType,
			LPDWORD lpdwPartition, LPDWORD lpdwOffsetLow,
			LPDWORD lpdwOffsetHigh )
{ 
  FIXME("(%04x, %ld, %p, %p, %p) stub!\n", hDevice, dwPositionType,
    lpdwPartition, lpdwOffsetLow, lpdwOffsetHigh);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		GetTapeStatus  (KERNEL32.411)
 */
DWORD WINAPI GetTapeStatus( HANDLE hDevice )
{
  FIXME("(%04x) stub!\n", hDevice);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		PrepareTape  (KERNEL32.554)
 */
DWORD WINAPI PrepareTape( HANDLE hDevice,  DWORD dwOperation,  BOOL bImmediate )
{ 
  FIXME("(%04x, %ld, %d) stub!\n", hDevice, dwOperation, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		SetTapeParameters  (KERNEL32.667)
 */
DWORD WINAPI SetTapeParameters( HANDLE hDevice, DWORD dwOperation,
                         LPVOID lpTapeInformation )
{
  FIXME("(%04x, %ld, %p) stub!\n", hDevice, dwOperation, lpTapeInformation);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		SetTapePosition  (KERNEL32.668)
 */
DWORD WINAPI SetTapePosition( HANDLE hDevice, DWORD dwPositionMethod, DWORD
                       dwPartition, DWORD dwOffsetLow, DWORD dwOffsetHigh,
                       BOOL bImmediate )
{
  FIXME("(%04x, %ld, %ld, %ld, %ld, %d) stub!\n", hDevice, dwPositionMethod,
    dwPartition, dwOffsetLow, dwOffsetHigh, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}


/************************************************************************
 *		WriteTapemark  (KERNEL32.751)
 */
DWORD WINAPI WriteTapemark( HANDLE hDevice,  DWORD dwTapemarkType, DWORD
                     dwTapemarkCount, BOOL bImmediate )
{
  FIXME("(%04x, %ld, %ld, %d) stub!\n", hDevice, dwTapemarkType,
    dwTapemarkCount, bImmediate);

  SetLastError( ERROR_NOT_SUPPORTED );

  return FALSE;
}
